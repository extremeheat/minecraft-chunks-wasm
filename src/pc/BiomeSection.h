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

  void setBiomeId(const Vec3 &pos, int biome) { blocks[getIndex(pos)] = biome; }

  int getBiomeId(const Vec3 &pos) { return blocks[getIndex(pos)]; }

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
    assert(dataLength == storage.wordsCount,
           "biome palette dataLength does not match expected");
    // printf("Biome data len %d %d ; palette len %d\n", dataLength, storage.byteSize, paletteLength);
    storage.read(stream);

    for (int i = 0; i < 4 * 4 * 4; i++) {
      blocks[i] = storage.get(i);
    }
  }

  void write(BinaryStream &stream) {
    // Build palette
    u8 counts[256]{0};
    u16 palette[64]{0};
    u16 positionInPalette[64]{0};
    int paletteLength = 0;

    for (int i = 0; i < 64; i++) {
      auto block = blocks[i];
      if (++counts[block] == 1) {
        positionInPalette[block] = paletteLength;
        palette[paletteLength++] = block;
      }
    }
    // Write palette

    auto bitsPerBlock = log2ceil(paletteLength);

    if (!bitsPerBlock) {
      stream.writeByte(0);             // bits per block
      stream.writeVarInt(palette[0]);  // palette
      stream.writeByte(0);             // palette length
      return;
    }

    stream.writeByte(bitsPerBlock);

    stream.writeVarInt(paletteLength);
    for (int i = 0; i < paletteLength; i++) {
      stream.writeVarInt(palette[i]);
    }

    auto storage = PalettedStorage<u64>(bitsPerBlock, 4 * 4 * 4);
    stream.writeVarInt(storage.wordsCount);  // palette length
    for (int i = 0; i < 64; i++) {
      storage.set(i, positionInPalette[blocks[i]]);
    }

    storage.write(stream);
  }
};