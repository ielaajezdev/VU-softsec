#ifndef COOKING_H
#define COOKING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#define MAX_NAME_LEN 32
#define MAX_EFFECT_LEN 32

class Dish;
class Ingredient;
class Meat;
class Spice;
class Vegetable;

enum IngredientState {
    RAW,
    COOKED,
    BURNED,
};

enum IngredientType {
    PLAIN,
    MEAT,
    SPICE,
    VEGETABLE,
};

enum MeatType {
    PORK,
    CHICKEN,
    BEEF,
};

std::string StateToString(IngredientState state);


class Dish {
private:
        size_t meat_cnt;
        size_t veggie_cnt;
        size_t calories;
        IngredientState meat_state;
        IngredientState veggie_state;
        Spice *spice;
        bool finished;

public:
    Dish(void) : meat_cnt(0), veggie_cnt(0), calories(0), meat_state(RAW),
        veggie_state(RAW), spice(nullptr), finished(false) {}
    ~Dish() {}

    bool Heat(unsigned int temp);

    bool AddIngredient(Ingredient *ingredient);

    void Complete();
};

class Ingredient {
protected:
        char *name;
        IngredientType type;

public:
    Ingredient(char *name) : type(PLAIN) {
        this->name = (char *) calloc(MAX_NAME_LEN, 1);
        if (strlen(name) <= MAX_NAME_LEN-1) {
            strncpy(this->name, name, strlen(name));
        }
    }

    ~Ingredient() {
        if (this->name) {
            free(this->name);
        }
    }

    IngredientType GetType() {
        return this->type;
    }

    char *GetName() {
        return this->name;
    }

    bool SetName(char *name) {
        if (strlen(name) > MAX_NAME_LEN-1) {
            return false;
        }

        memset(this->name, 0, MAX_NAME_LEN);
        strncpy(this->name, name, strlen(name));
        return true;
    }
};

class BasicIngredient : public Ingredient {
private:
    size_t calories;

public:
    BasicIngredient(char *name, size_t calories, IngredientType type)
        : Ingredient(name) {
        this->type = type;
        this->calories = calories;
    }

    size_t GetCalories() {
        return this->calories;
    }

    void SetCalories(size_t calories) {
        this->calories = calories;
    }
};

class Spice : public Ingredient {
private:
    char *special_effect;

public:
    Spice(char *name, char *special_effect) : Ingredient(name) {
        this->type = SPICE;
        this->special_effect = (char *) calloc(MAX_EFFECT_LEN, 1);

        if (strlen(special_effect) <= MAX_EFFECT_LEN-1) {
            strncpy(this->special_effect, special_effect, strlen(special_effect));
        }
    }

    ~Spice() {
        if (this->special_effect) {
            free(this->special_effect);
        }
    }

    char *GetSpecialEffect() {
        return this->special_effect;
    }

    bool SetSpecialEffect(const char *special_effect) {
        if (strlen(special_effect) > MAX_EFFECT_LEN-1) {
            return false;
        }

        memset(this->special_effect, 0, MAX_EFFECT_LEN);
        strncpy(this->special_effect, special_effect, strlen(special_effect));
        return true;
    }
};

#endif  // COOKING_H
