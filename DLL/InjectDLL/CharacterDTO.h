#pragma once

#include "CharacterEquipment.h"
#include "CharacterLocation.h"
#include "Quaternion.h"
#include "Vec3f.h"
#include <cstdint>
#include <string>
#include <vector>

using namespace DataTypes;

namespace DTO
{

class ClientCharacterDTO
{
  public:
    Vec3f Position;
    Quaternion Rotation1;
    Quaternion Rotation2;
    Quaternion Rotation3;
    Quaternion Rotation4;
    int Animation;
    int Health;
    float AtkUp;
    bool IsEquipped;
    CharacterEquipment Equipment;
    CharacterLocation Location;
    Vec3f Bomb;
    Vec3f Bomb2;
    Vec3f BombCube;
    Vec3f BombCube2;
};

class CloseCharacterDTO
{
  public:
    uint8_t PlayerNumber;
    uint8_t Status;
    bool Updated;
    Vec3f Position;
    Quaternion Rotation1;
    Quaternion Rotation2;
    Quaternion Rotation3;
    Quaternion Rotation4;
    int Animation;
    int Health;
    float AtkUp;
    bool IsEquipped;
    CharacterEquipment Equipment;
    CharacterLocation Location;
    Vec3f Bomb;
    Vec3f Bomb2;
    Vec3f BombCube;
    Vec3f BombCube2;
};

class FarCharacterDTO
{
  public:
    uint8_t PlayerNumber;
    uint8_t Status;
    bool Updated;
    Vec3f Position;
    CharacterLocation Location;
};

} // namespace DTO