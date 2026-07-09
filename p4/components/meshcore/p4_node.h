/*
 * p4_node.h — minimal headless MeshCore node for the T-Display P4 (M3.5).
 *
 * Brings up a BaseChatMesh node over the SX1262 radio: load-or-create the
 * ed25519 identity (persisted via IdentityStore on the fs_shim), start the
 * mesh, broadcast a self-advert, and run the mesh loop on its own task.
 * Full MyMesh + MeshProxy + UITask replace this at M4.
 *
 * Safe to include from main.cpp (no cpp_bus_driver / mesh headers leak here).
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Mount storage, create/load identity, start the mesh + advert, spawn the loop
// task. Call once after the radio is attached (meck_radio_attach()).
void p4_node_start(void);

#ifdef __cplusplus
}
#endif
