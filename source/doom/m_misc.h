//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
//-----------------------------------------------------------------------------
#pragma once

#include "types/strings.h"
#include "types/numbers.h"
#include "containers/vector.h"
#include "utility/convert.h"

bool M_WriteFile(const filesys::path&, const char* source, uint32 length);
vector<byte> M_ReadFile(const filesys::path& path);

void M_ScreenShot();

class SettingBase
{
public:
    SettingBase(string_view name);

    string_view Name() const { return name; }

    using VariantType = std::variant<int32, float, bool, string>;

    template<typename AS>
    std::optional<AS> GetAs() const
    {
        auto* out = std::get_if<AS>(GetVar());
        return out ? std::optional{*out} : std::nullopt;
    }

    virtual VariantType GetVar() const = 0;
    virtual void SetVar(const VariantType& in) = 0;
    virtual void Set(string_view in) = 0;

    virtual void Reset() = 0;

protected:
    string name;
    bool isSet = false;
};

template<typename T>
class Setting : public SettingBase
{
public:
    Setting(string_view name, T defaultValue)
        : SettingBase{name}
        , value{defaultValue}
        , defaultValue{defaultValue}
    {}

    Setting() = delete;
    Setting(const Setting&) = delete;
    Setting(Setting&&) = delete;
    Setting& operator=(const Setting&) = delete;

    template<typename AS>
    AS GetAs() const { return convert<AS>(value); }

    T Get() const { return value; }

    VariantType GetVar() const override
    {
        return value;
    }

    void SetVar(const VariantType& in) override
    {
        auto* inVal = get_if<T>(&in);
        if (inVal)
            value = *inVal;
    }

    void Set(T in)
    {
        value = in;
        isSet = true;
    }

    void Set(string_view in) override
    {
        value = convert<T>(in);
        isSet = true;
    }

    void Reset() override { value = defaultValue; isSet = false; }

    operator T() { return value; }

    T operator=(const T&& other) { value = other; return value; }

    Setting<T>& operator++() { static_assert(is_integral<T>); value += 1; return *this; }
    Setting<T>& operator--() { static_assert(is_integral<T>); value -= 1; return *this; }
    T operator++(int) { static_assert(is_integral<T>); auto out = value; value += 1; return out; }
    T operator--(int) { static_assert(is_integral<T>); auto out = value; value -= 1; return out; }

private:
    T value;
    T defaultValue;
};

template<typename T>
T operator-(const T&& a, const Setting<T>& b) { return a - b.Get(); }

class Settings
{
public:
    static const filesys::path DevDataPath;
    static const filesys::path DevMapPath;

    static void Save(const filesys::path& path = DefaultConfigFile);
    static void Load();

    template<typename T>
    static std::optional<T> Get(string_view name)
    {
        auto it = settings.find(name);
        return (it == settings.end()) ? std::nullopt : it->get_if<T>();
    }

    static void Register(SettingBase* setting)
    {
        Directory().insert({setting->Name(), setting});
    }

private:
    using DirectoryType = std::map<string_view, SettingBase*>;

    static DirectoryType& Directory()
    {
        if (!settings)
            settings = new DirectoryType;

        return *settings;
    }

    static const filesys::path DefaultConfigFile;

    filesys::path fileName;

    static DirectoryType* settings;
};

extern Setting<int32> screenBlocks;
extern Setting<int32> detailLevel;

extern Setting<int32> key_right;
extern Setting<int32> key_right;
extern Setting<int32> key_left;
extern Setting<int32> key_up;
extern Setting<int32> key_down;

extern Setting<int32> key_strafeleft;
extern Setting<int32> key_straferight;

extern Setting<int32> key_fire;
extern Setting<int32> key_use;
extern Setting<int32> key_strafe;
extern Setting<int32> key_speed;

extern Setting<int32> numChannels;
extern Setting<int32> usegamma;
