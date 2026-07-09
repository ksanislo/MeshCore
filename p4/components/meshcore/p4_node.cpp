/*
 * p4_node.cpp — minimal headless MeshCore node (M3.5).
 *
 * A thin BaseChatMesh subclass with stub app callbacks: enough to create an
 * identity, join the mesh, advertise, and log received adverts/messages. This
 * proves the whole MeshCore software stack (Dispatcher, ed25519 crypto, the
 * fs_shim-backed IdentityStore) runs on the P4 over our cpp_bus_driver radio.
 * Full MyMesh + MeshProxy + UITask supersede this at M4.
 */
#include "p4_node.h"
#include "p4_radio.h"                       // p4_get_radio()
#include <helpers/BaseChatMesh.h>
#include <helpers/ArduinoHelpers.h>         // ArduinoMillis, VolatileRTCClock
#include <helpers/SimpleMeshTables.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/IdentityStore.h>
#include "FS.h"                             // fs_shim: InternalFS, fs_mount_spiffs()
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include <stdio.h>
#include <string.h>

// ---- Hardware TRNG-backed RNG ------------------------------------------------
class P4RNG : public mesh::RNG {
public:
  void begin() { }
  void random(uint8_t* dest, size_t sz) override { esp_fill_random(dest, sz); }
};

// ---- Minimal chat mesh: stub every app callback -----------------------------
class MinimalMesh : public BaseChatMesh {
public:
  MinimalMesh(mesh::Radio& radio, mesh::MillisecondClock& ms, mesh::RNG& rng,
              mesh::RTCClock& rtc, mesh::PacketManager& mgr, mesh::MeshTables& tables)
    : BaseChatMesh(radio, ms, rng, rtc, mgr, tables) { }

  // Public entry to the (protected) BaseChatMesh::begin() chain.
  void start() { BaseChatMesh::begin(); }

protected:
  void onDiscoveredContact(ContactInfo& c, bool is_new, uint8_t path_len,
                           const uint8_t* path) override {
    printf("[node] RX advert: contact '%s' (new=%d, path_len=%u)\n",
           c.name, (int)is_new, (unsigned)path_len);
  }

  ContactInfo* processAck(const uint8_t* data) override { return NULL; }
  void onContactPathUpdated(const ContactInfo& c) override { }

  void onMessageRecv(const ContactInfo& c, mesh::Packet* pkt,
                     uint32_t sender_timestamp, const char* text) override {
    printf("[node] RX msg from '%s': %s\n", c.name, text ? text : "");
  }
  void onCommandDataRecv(const ContactInfo& c, mesh::Packet* pkt,
                         uint32_t sender_timestamp, const char* text) override { }
  void onSignedMessageRecv(const ContactInfo& c, mesh::Packet* pkt,
                           uint32_t sender_timestamp, const uint8_t* sender_prefix,
                           const char* text) override { }

  uint32_t calcFloodTimeoutMillisFor(uint32_t pkt_airtime_millis) const override {
    return 12000 + 2 * pkt_airtime_millis;
  }
  uint32_t calcDirectTimeoutMillisFor(uint32_t pkt_airtime_millis, uint8_t path_len) const override {
    return 8000 + (pkt_airtime_millis * 4 + 300) * ((path_len & 63) + 1);
  }
  void onSendTimeout() override { }

  void onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt,
                            uint32_t timestamp, const char* text) override {
    printf("[node] RX channel msg: %s\n", text ? text : "");
  }

  uint8_t onContactRequest(const ContactInfo& c, uint32_t sender_timestamp,
                           const uint8_t* data, uint8_t len, uint8_t* reply) override {
    return 0;
  }
  void onContactResponse(const ContactInfo& c, const uint8_t* data, uint8_t len) override { }
};

// ---- Collaborators (static so they outlive app_main) ------------------------
static ArduinoMillis            s_ms;
static P4RNG                    s_rng;
static VolatileRTCClock         s_rtc;
static SimpleMeshTables         s_tables;
static StaticPoolPacketManager  s_mgr(16);
static MinimalMesh*             s_mesh = nullptr;

static const char* NODE_NAME = "P4-node";

static void mesh_task(void* arg) {
  // Broadcast a self-advert once we're running so nearby nodes discover us.
  mesh::Packet* adv = s_mesh->createSelfAdvert(NODE_NAME);
  if (adv) {
    s_mesh->sendFlood(adv);
    printf("[node] self-advert broadcast as '%s'\n", NODE_NAME);
  } else {
    printf("[node] createSelfAdvert failed\n");
  }

  uint32_t readvert_ms = 0;
  while (true) {
    s_rtc.tick();       // advance the soft clock
    s_mesh->loop();     // drives Dispatcher -> radio.recvRaw + outbound queue

    // Re-advertise every 60 s so a node that boots later still hears us.
    if (++readvert_ms >= 6000) {
      readvert_ms = 0;
      mesh::Packet* a = s_mesh->createSelfAdvert(NODE_NAME);
      if (a) s_mesh->sendFlood(a);
      printf("[node] periodic advert (heap free check ok)\n");
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void p4_node_start(void) {
  // Storage is mounted by app_main before radio power-up (see main.cpp).

  // Load or create the node identity.
  IdentityStore id_store(fs::InternalFS, "/identity");
  id_store.begin();

  mesh::LocalIdentity self_id;
  if (id_store.load("_main", self_id)) {
    printf("[node] loaded existing identity\n");
  } else {
    printf("[node] no identity found; generating a new one\n");
    int tries = 0;
    do {
      self_id = mesh::LocalIdentity(&s_rng);
    } while (++tries < 10 &&
             (self_id.pub_key[0] == 0x00 || self_id.pub_key[0] == 0xFF));
    if (!id_store.save("_main", self_id)) {
      printf("[node] WARNING: failed to persist identity\n");
    }
  }

  char pub[80];
  for (int i = 0; i < PUB_KEY_SIZE; i++) sprintf(pub + i * 2, "%02x", self_id.pub_key[i]);
  printf("[node] identity pubkey: %s\n", pub);

  // Build the mesh and hand it the identity.
  s_mesh = new MinimalMesh(p4_get_radio(), s_ms, s_rng, s_rtc, s_mgr, s_tables);
  s_mesh->self_id = self_id;
  s_mesh->start();
  printf("[node] mesh started; spawning mesh task\n");

  xTaskCreate(mesh_task, "mesh_task", 16 * 1024, NULL, 5, NULL);
}
