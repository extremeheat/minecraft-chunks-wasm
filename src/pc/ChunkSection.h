#pragma once
#include "../PalettedStorage.h"
#include "../Registry.h"

struct ChunkSection {
  short blocks[4096]{0};
  short palette[4096]{0};
  // if this is zero this section does not exist
  int paletteLength = 0;
  int occupiedBlocks;

  Registry *registry = nullptr;

  ChunkSection() {}

  ChunkSection(Registry *registry) : registry(registry) {}

  inline bool isEmpty() { return paletteLength == 0; }

  inline int getIndex(const Vec3 &pos) {
    return (pos.y & 0xf) << 8 | (pos.z & 0xf) << 4 | (pos.x & 0xf);
  }

  void setBlockStateId(const Vec3 &pos, int stateId) {
    blocks[getIndex(pos)] = stateId;
  }

  int getBlockStateId(const Vec3 &pos) { return blocks[getIndex(pos)]; }

  void read(BinaryStream &stream) {
    this->occupiedBlocks = stream.readShortBE();
    assert(occupiedBlocks <= 4096);
    u8 bitsPerBlock = stream.readByte();
    assert(bitsPerBlock < 16);

    if (!bitsPerBlock) {
      this->paletteLength = 1;
      this->palette[0] = stream.readVarInt();
      assert(stream.readByte() == 0,
             "Expected to read 0 length data for 1 length palette");
      return;
    }

    this->paletteLength = stream.readVarInt();
    for (int i = 0; i < this->paletteLength; i++) {
      this->palette[i] = stream.readVarInt();
    }

    auto dataLength = stream.readVarInt();
    auto storage = PalettedStorage<u64>(bitsPerBlock);
    storage.read(stream);

    for (int i = 0; i < 4096; i++) {
      blocks[i] = storage.get(i);
    }
  }

  void write(BinaryStream &stream) {
    auto bitsPerBlock = log2ceil(this->paletteLength);

    if (!bitsPerBlock) {
      stream.writeShortBE(0);                // occupied blocks
      stream.writeByte(0);                   // bits per block
      stream.writeVarInt(this->palette[0]);  // palette
      stream.writeByte(0);                   // palette length
      return;
    }

    stream.writeShortBE(occupiedBlocks);
    stream.writeByte(bitsPerBlock);
    stream.writeVarInt(this->paletteLength);
    for (int i = 0; i < this->paletteLength; i++) {
      stream.writeVarInt(this->palette[i]);
    }

    auto paletteLength = 16 * 16 * 16;
    stream.writeVarInt(paletteLength);
    auto storage = PalettedStorage<u64>(bitsPerBlock, paletteLength);

    for (int i = 0; i < 4096; i++) {
      storage.set(i, blocks[i]);
    }

    storage.write(stream);
  }
};
