#pragma once

#include <cstdint>

namespace DataTypes
{

class CharacterEquipment
{
  public:
    uint8_t WType;
    short Sword;
    short Shield;
    short Bow;
    short Head;
    short Upper;
    short Lower;
};

} // namespace DataTypes