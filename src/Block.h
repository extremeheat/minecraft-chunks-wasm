#pragma once
#include "./Types.h"

struct BlockEntity {
  const char *tag;
  int tagLength;
  Vec3 position;

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