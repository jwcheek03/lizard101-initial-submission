#pragma once
#include <string>
#include <vector>
#include <functional>
#include <random>

// Forward declarations
struct CombatPlayer;
struct CardGameState;
struct CardDefinition;

// ---- Card / stat enums ----
enum class CardType { Spell, Aura, Trap };
enum class School { Generic, Fire, Water, Plant };

// What kind of targeting the card requires
enum class TargetScope {
    None,           // e.g. "gain 2 mana"
    Self,           // affect only the caster
    SingleEnemy,    // Targets a single enemy
    AllEnemies,     // Targets all enemies
    SingleAlly,     // Targets a single ally
    AllAllies,      // Targets all allies
    AnySingle,      // could be ally or enemy
    Everyone        // Targets all allies and enemies
};

// Which "zone" a set of cards represents
enum class ZoneType { Deck, Hand, Discard };


// Persistent aura types (extend later as needed)
enum class AuraKind {
    None,
    RainyPresence,
    RampantGrowth,
    FireSpitter,
    Waterfall,
    DeepRoots
};

struct ActiveAura {
    AuraKind kind = AuraKind::None;
    int      remainingTurns = 0;  // decremented at end of character's turn
};




enum class TrapKind {
    None,
    ExplosiveTrap,
    SnakeTrap,
    IceBlock,
    SnuffOut,
    SpikeTrap
};

struct TrapInstance {
    TrapKind               kind = TrapKind::None;
    const CardDefinition* def = nullptr;
    int                    instanceId = 0; // for debugging
};




// ---- Stats ----
struct Stats {
    int attack = 0;
    int defense = 0;
    int burn = 0;
    int healBonus = 0;

    // Apply end-of-turn logic:
    //  - burn deals damage equal to its current value, then burns down by 1
    //  - attack, defense, and healBonus simply decay by 1 if positive.
    void endOfTurn(CombatPlayer& owner);
};

// ---- Effects ----
struct EffectContext {
    CardGameState& state;
    CombatPlayer& source;
    std::vector<CombatPlayer*>  targets;
};

using EffectFn = std::function<void(const EffectContext&)>;

// ---- Card definition & instances ----
struct CardDefinition {
    int          id = 0;
    std::string  name;
    CardType     type = CardType::Spell;
    School       school = School::Generic;
    int          manaCost = 0;
    TargetScope  targetScope = TargetScope::None;
    std::string  texturePath;  // relative path in /media, "media/cards/fireball.png"
    EffectFn     effect;       // executes the card’s effect

    TrapKind     trapKind = TrapKind::None;
};

struct CardInstance {
    const CardDefinition* def = nullptr;
    int                   instanceId = 0; // unique per physical card instance
    bool                  ephemeral = false;
};

// ---- Zones: Deck / Hand / Discard ----
struct CardZone {
    ZoneType type;
    std::vector<CardInstance> cards;

    explicit CardZone(ZoneType t) : type(t) {}

    bool   empty() const { return cards.empty(); }
    size_t size()  const { return cards.size(); }

    // Add on top (back of vector used as top)
    void addToTop(const CardInstance& c) {
        cards.push_back(c);
    }

    // Remove and return top card. Throws if empty.
    CardInstance drawFromTop();
};

// ---- Play queue ----
struct QueuedCard {
    CardInstance                    card;
    std::vector<CombatPlayer*>      targets;
};

struct PlayQueue {
    std::vector<QueuedCard> items;

    bool empty() const { return items.empty(); }

    void enqueue(const CardInstance& c, const std::vector<CombatPlayer*>& targets) {
        items.push_back(QueuedCard{ c, targets });
    }

    QueuedCard popFront();
};

// ---- Player in combat ----
struct CombatPlayer {
    std::string name;
    School      school = School::Generic;
    int         maxHp = 20;
    int         hp = 20;
    int         mana = 0;
    int         fatigueCounter = 0;    // increases whenever player tries to draw from an empty deck
    Stats       stats;

    ActiveAura  aura;

    std::vector<TrapInstance> traps;
    int cardsPlayedThisTurn = 0;
    bool stunned = false;
    bool stunProtected = false;

    std::vector<const CardDefinition*> earnedRewardCards; 

    CardZone    deck;
    CardZone    hand;
    CardZone    discard;
    PlayQueue   queue;

    CombatPlayer(const std::string& n, School s, int maxHp_);

    void startTurn(CardGameState& state);
    void endTurn();

    void gainMana(int amount);
    void drawCards(int count, std::mt19937& rng);

    void takeDamage(int rawDamage); // applies defense + logs
    bool isDead() const { return hp <= 0; }
};

// ---- Overall game state ----
struct CardGameState {
    std::vector<CombatPlayer> players;
    std::vector<CombatPlayer> enemies;

    int          activePlayerIndex = 0;
    std::mt19937 rng;

    bool   firstRound = true;
    int    firstRoundTurnsRemaining = 0;

    int    enemyTurnIndex = -1;
    bool   enemyTurnPending = false;
    double enemyTurnPauseTime = 0.0;

    int    nextInstanceIdGlobal = 100000;
    int    nextTrapId = 1;

    CardGameState();

    CombatPlayer& activePlayer();

    void applyAura(CombatPlayer& p, AuraKind kind);

    // Turn flow
    void startTurn();
    void startEnemyTurn();
    void endTurn();

    // Returns false if illegal (index out of range, not enough mana, etc.)
    bool playCardFromHand(size_t handIndex,
        const std::vector<CombatPlayer*>& targets);

    // Called internally after queuing a card
    void resolveQueue();

    void dealStartingHands(int cards);

    void conjureCardToHand(CombatPlayer& owner,
        const std::string& cardName,
        bool ephemeral = true);

    bool tryCounterWithSnuffOut(CombatPlayer& actor,
        const CardInstance& card);

    void triggerOnAttackedTraps(CombatPlayer& attacker,
        CombatPlayer& victim,
        int damageDealt);

    void triggerOnCardPlayedTraps(CombatPlayer& actor);

    void triggerOnEndOfTurnTraps(CombatPlayer& actor);

    void addTrap(CombatPlayer& owner, const CardDefinition& def);

    void onDirectDamage(CombatPlayer& source,
        CombatPlayer& target,
        int damageDealt);

    void onHeal(CombatPlayer& source,
        CombatPlayer& target,
        int amountHealed);
};


// ---- Card "database" helpers ----
namespace LizardCards {

    // Create core definitions and return a reference to the storage.
    const std::vector<CardDefinition>& getAllCards();

    const CardDefinition* findByName(const std::string& name);

    // Simple starting decks per school.
    void buildStartingDeck(School school,
        std::vector<CardInstance>& outDeck,
        int& nextInstanceId);
}
