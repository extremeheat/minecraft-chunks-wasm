#include "pc/ChunkColumn.h"

// Some simple bindings curtsey of copilot

#ifdef WEBASSEMBLY
#define EXPORT(name) __attribute__((export_name(#name))) name
#else
#define EXPORT(name) name
#endif

extern "C" {

void *EXPORT(pc118_loadChunkPacket)(u8 *buffer, int length) {
  auto cc = ChunkColumn::readChunkPacket(0, buffer, length);
  return cc;
}

int EXPORT(pc118_getBlockStateId)(void *cc, int x, int y, int z) {
  auto chunkColumn = (ChunkColumn *)cc;
  return chunkColumn->getBlockStateId({x, y, z});
}

void EXPORT(pc118_setBlockStateId)(void *cc, int x, int y, int z, int stateId) {
  auto chunkColumn = (ChunkColumn *)cc;
  chunkColumn->setBlockStateId({x, y, z}, stateId);
}

int EXPORT(pc118_getSkyLight)(void *cc, int x, int y, int z, int skyLight) {
  auto chunkColumn = (ChunkColumn *)cc;
  return chunkColumn->getSkyLight({x, y, z});
}

void EXPORT(pc118_setSkyLight)(void *cc, int x, int y, int z, int skyLight) {
  auto chunkColumn = (ChunkColumn *)cc;
  chunkColumn->setSkyLight({x, y, z}, skyLight);
}

int EXPORT(pc118_getBlockLight)(void *cc, int x, int y, int z, int blockLight) {
  auto chunkColumn = (ChunkColumn *)cc;
  return chunkColumn->getBlockLight({x, y, z});
}

void EXPORT(pc118_setBlockLight)(void *cc, int x, int y, int z,
                                 int blockLight) {
  auto chunkColumn = (ChunkColumn *)cc;
  chunkColumn->setBlockLight({x, y, z}, blockLight);
}

int EXPORT(pc118_getBiomeId)(void *cc, int x, int z) {
  auto chunkColumn = (ChunkColumn *)cc;
  return chunkColumn->getBiomeId({x, z});
}

void EXPORT(pc118_setBiomeId)(void *cc, int x, int z, int biomeId) {
  auto chunkColumn = (ChunkColumn *)cc;
  chunkColumn->setBiomeId({x, z}, biomeId);
}

int EXPORT(pc118_hasBlockEntity)(void *cc, int x, int y, int z) {
  auto chunkColumn = (ChunkColumn *)cc;
  auto entity = chunkColumn->getBlockEntity({x, y, z});
  if (entity) return entity->tagLength;
  return 0;
}

const i8 *EXPORT(pc118_getBlockEntity)(void *cc, int x, int y, int z) {
  auto chunkColumn = (ChunkColumn *)cc;
  auto entity = chunkColumn->getBlockEntity({x, y, z});
  if (entity) return entity->tag;
  return nullptr;
}

void EXPORT(pc118_setBlockEntity)(void *cc, int x, int y, int z, const i8 *tag,
                                  int tagLength) {
  auto chunkColumn = (ChunkColumn *)cc;
  chunkColumn->setBlockEntity({x, y, z}, {tag, tagLength});
}
}