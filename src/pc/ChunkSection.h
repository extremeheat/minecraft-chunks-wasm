#pragma once
#include "../PalettedStorage.h"
#include "../Registry.h"

struct ChunkSection {
  short blocks[4096];
  int empty = true;
  int occupiedBlocks;

  Registry *registry = nullptr;

  ChunkSection() {}

  ChunkSection(Registry *registry) : registry(registry) {}

  inline bool isEmpty() { return empty; }

  inline int getIndex(const Vec3i &pos) {
    return (pos.y & 0xf) << 8 | (pos.z & 0xf) << 4 | (pos.x & 0xf);
  }

  void setBlockStateId(const Vec3i &pos, int stateId) {
    blocks[getIndex(pos)] = stateId;
  }

  int getBlockStateId(const Vec3i &pos) { return blocks[getIndex(pos)]; }

  void read(BinaryStream &stream) {
    this->occupiedBlocks = stream.readShortBE();
    // assert(occupiedBlocks <= 4096);
    u8 bitsPerBlock = stream.readByte();
    assert(bitsPerBlock < 16);

    int paletteLength;
    short palette[4096];

    if (!bitsPerBlock) {
      paletteLength = 1;
      palette[0] = stream.readVarInt();
      assert(stream.readByte() == 0,
             "Expected to read 0 length data for 1 length palette");
      return;
    }

    paletteLength = stream.readVarInt();
    for (int i = 0; i < paletteLength; i++) {
      palette[i] = stream.readVarInt();
    }

    auto dataLength = stream.readVarInt();
    auto storage = PalettedStorage<u64>(bitsPerBlock);
    storage.read(stream);

    for (int i = 0; i < 4096; i++) {
      blocks[i] = palette[storage.get(i)];
    }
  }

  void write(BinaryStream &stream) {
    // Build palette
    u8 counts[30'000]{0};  // assuming max block states is <30'000
    u16 palette[4096]{0};
    u16 positionInPalette[4096]{0};
    int paletteLength = 0;

    for (int i = 0; i < 4096; i++) {
      auto block = blocks[i];
      if (++counts[block] == 1) {
        positionInPalette[block] = paletteLength;
        palette[paletteLength++] = block;
      }
    }

    // Write palette
    auto bitsPerBlock = log2ceil(paletteLength);

    if (!bitsPerBlock) {
      stream.writeShortBE(0);          // occupied blocks
      stream.writeByte(0);             // bits per block
      stream.writeVarInt(palette[0]);  // palette
      stream.writeByte(0);             // data length
      return;
    }

    stream.writeShortBE(occupiedBlocks);
    stream.writeByte(bitsPerBlock);

    stream.writeVarInt(paletteLength);
    for (int i = 0; i < paletteLength; i++) {
      stream.writeVarInt(palette[i]);
    }

    auto storage = PalettedStorage<u64>(bitsPerBlock, 4096);
    stream.writeVarInt(storage.wordsCount);  // palette length
    for (int i = 0; i < 4096; i++) {
      storage.set(i, positionInPalette[blocks[i]]);
    }

    storage.write(stream);
  }
};
