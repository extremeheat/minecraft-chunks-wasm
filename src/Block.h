#pragma once
#include "./Types.h"

struct BlockEntity {
  const char *tag = nullptr;
  int tagLength = 0;
  Vec3 position;

  BlockEntity(const i8 *tag, int tagLength) : tag(tag), tagLength(tagLength) {}

  BlockEntity(BlockEntity *other) {
    this->tag = other->tag;
    this->tagLength = other->tagLength;
    this->position = other->position;
  }
};

struct Block {
  int stateId;
  int biomeId;
  int blockLight;
  int skyLight;
  BlockEntity *blockEntity;
};