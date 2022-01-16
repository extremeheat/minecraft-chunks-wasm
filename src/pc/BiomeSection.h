#pragma once
#include "../PalettedStorage.h"
#include "../Registry.h"

class BiomeSection {
 public:
  short blocks[64]{0};
  short palette[64]{0};
  int paletteLength = 0;

  Registry *registry = nullptr;

  BiomeSection() {}
  BiomeSection(Registry *registry) : registry(registry) {}

  inline bool isEmpty() { return paletteLength == 0; }

  inline int getIndex(const Vec3 &pos) {
    return (pos.y << 4) | (pos.z << 2) | pos.x;
  }

  void read(BinaryStream &stream) {
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
    auto storage = PalettedStorage<u64>(bitsPerBlock, 4 * 4 * 4);
    storage.read(stream);

    for (int i = 0; i < 4 * 4 * 4; i++) {
      blocks[i] = storage.get(i);
    }
  }

  void write(BinaryStream &stream) {
    auto bitsPerBlock = log2ceil(this->paletteLength);

    if (!bitsPerBlock) {
      stream.writeByte(0);                   // bits per block
      stream.writeVarInt(this->palette[0]);  // palette
      stream.writeByte(0);                   // palette length
      return;
    }

    stream.writeByte(bitsPerBlock);
    stream.writeVarInt(this->paletteLength);
    for (int i = 0; i < this->paletteLength; i++) {
      stream.writeVarInt(palette[i]);
    }

    auto paletteLength = 4 * 4 * 4;
    stream.writeVarInt(paletteLength);
    auto storage = PalettedStorage<u64>(bitsPerBlock, paletteLength);

    for (int i = 0; i < 4 * 4 * 4; i++) {
      storage.set(i, blocks[i]);
    }

    storage.write(stream);
  }

  void setBiomeId(const Vec3 &pos, int biome) { blocks[getIndex(pos)] = biome; }

  int getBiomeId(const Vec3 &pos) { return blocks[getIndex(pos)]; }
};