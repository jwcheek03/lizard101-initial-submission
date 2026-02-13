#include "game/Lizard101Core.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

// ---------------- STATS ----------------
void Stats::endOfTurn(CombatPlayer& owner) {
    // Burn: deal damage equal to current burn, then decrement if > 0
    if (burn > 0) {
        owner.takeDamage(burn);
        std::cout << "[BURN] " << owner.name << " takes " << burn
            << " burn damage, HP " << owner.hp << "/" << owner.maxHp << "\n";
        burn -= 1;
    }

    // Temporary buffs decay
    if (attack > 0)    attack -= 1;
    if (defense > 0)   defense -= 1;
    if (healBonus > 0) healBonus -= 1;
}



// ---------------- ZONE ----------------
CardInstance CardZone::drawFromTop() {
    if (cards.empty()) {
        throw std::runtime_error("Attempted to draw from empty zone");
    }
    CardInstance c = cards.back();
    cards.pop_back();
    return c;
}

// ---------------- PLAY QUEUE ----------------
QueuedCard PlayQueue::popFront() {
    if (items.empty()) {
        throw std::runtime_error("Attempted to pop from empty queue");
    }
    QueuedCard front = items.front();
    items.erase(items.begin());
    return front;
}

// ---------------- COMBAT PLAYER ----------------
CombatPlayer::CombatPlayer(const std::string& n, School s, int maxHp_)
    : name(n)
    , school(s)
    , maxHp(maxHp_)
    , hp(maxHp_)
    , mana(0)
    , fatigueCounter(0)
    , deck(ZoneType::Deck)
    , hand(ZoneType::Hand)
    , discard(ZoneType::Discard)
{
}

void CombatPlayer::gainMana(int amount) {
    mana += amount;
    if (mana < 0) mana = 0;
}

void CombatPlayer::takeDamage(int rawDamage) {
    int reduced = rawDamage - stats.defense;
    if (reduced < 0) reduced = 0;
    hp -= reduced;
    if (hp < 0) hp = 0;

    std::cout << "[DAMAGE] " << name << " takes " << reduced
        << " damage (raw " << rawDamage << ", def " << stats.defense
        << "), HP " << hp << "/" << maxHp << "\n";
}

void CombatPlayer::drawCards(int count, std::mt19937& rng) {
    (void)rng; // currently unused, hook for fancier logic later

    for (int i = 0; i < count; ++i) {
        if (deck.empty()) {
            // Fatigue: each time we would draw from an empty deck, take ever-increasing damage
            ++fatigueCounter;
            takeDamage(fatigueCounter);
            std::cout << "[FATIGUE] " << name
                << " tried to draw from empty deck, fatigue "
                << fatigueCounter << "\n";
            break; // stop drawing further cards
        }
        hand.addToTop(deck.drawFromTop());
    }
}

void CombatPlayer::startTurn(CardGameState& state) {
    cardsPlayedThisTurn = 0;
    int manaGain = 2;
    if (aura.kind == AuraKind::RampantGrowth) {
        manaGain += 2;
        std::cout << "[AURA] Rampant Growth: " << name
            << " gains +2 extra mana this turn.\n";
    }
    gainMana(manaGain);

    // Draw 1 card except during first global round (handled in CardGameState)
    if (!deck.empty() && !state.firstRound) {
        hand.addToTop(deck.drawFromTop());
    }
    else if (deck.empty() && !state.firstRound) {
        // Also apply fatigue if we tried to draw at start of turn
        ++fatigueCounter;
        takeDamage(fatigueCounter);
        std::cout << "[FATIGUE] " << name
            << " tried to draw at start of turn, fatigue "
            << fatigueCounter << "\n";
    }
}


void CombatPlayer::endTurn() {
    stats.endOfTurn(*this);

    if (aura.remainingTurns > 0) {
        aura.remainingTurns--;
        if (aura.remainingTurns <= 0) {
            std::cout << "[AURA EXPIRE] " << name << "'s aura ends.\n";
            aura.kind = AuraKind::None;
        }
    }

    if (stunned) {
        // This was a stunned turn: clear stun and enable 1-turn protection.
        stunned = false;
        stunProtected = true;
        std::cout << "[STUN] " << name
            << " is no longer stunned and becomes temporarily stun-immune.\n";
    }
    else if (stunProtected) {
        // This was the protected (normal) turn; protection now wears off.
        stunProtected = false;
        std::cout << "[STUN] " << name
            << "'s stun immunity wears off.\n";
    }
}



// ---------------- CARD GAME STATE ----------------
CardGameState::CardGameState()
    : rng(std::random_device{}())
{
}


void CardGameState::applyAura(CombatPlayer& p, AuraKind kind) {
    p.aura.kind = kind;
    p.aura.remainingTurns = 4; // cast turn + 3 more turns

    const char* auraName = nullptr;
    switch (kind) {
    case AuraKind::RainyPresence: auraName = "Rainy Presence"; break;
    case AuraKind::RampantGrowth: auraName = "Rampant Growth"; break;
    case AuraKind::FireSpitter:   auraName = "Fire Spitter";   break;
    case AuraKind::Waterfall:     auraName = "Waterfall";      break;
    default:                      auraName = "Unknown Aura";   break;
    }

    std::cout << "[AURA] " << p.name << " gains aura "
        << auraName << " for 4 turns.\n";
}


void CardGameState::onDirectDamage(CombatPlayer& source,
    CombatPlayer& target,
    int damageDealt)
{
    // Fire Spitter: "Whenever you deal damage to an enemy, apply 1 Burn to that enemy."
    if (damageDealt > 0 && source.aura.kind == AuraKind::FireSpitter) {
        target.stats.burn += 1;
        std::cout << "[AURA] Fire Spitter: " << source.name
            << " applies 1 Burn to " << target.name
            << " (now " << target.stats.burn << ").\n";
    }

    // When you are attacked traps (Explosive Trap, Ice Block, etc.)
    triggerOnAttackedTraps(source, target, damageDealt);

    // Waterfall: "Enemies target you with attacks, conjure 1 Icicle card to your hand whenever you are attacked."
    if (damageDealt > 0 && target.aura.kind == AuraKind::Waterfall) {
        conjureCardToHand(target, "Icicle", true);
    }
}





CombatPlayer& CardGameState::activePlayer() {
    if (players.empty()) {
        throw std::runtime_error("No players in CardGameState");
    }
    if (activePlayerIndex < 0 ||
        activePlayerIndex >= static_cast<int>(players.size())) {
        activePlayerIndex = 0;
    }
    return players[activePlayerIndex];
}

void CardGameState::startTurn() {
    CombatPlayer& p = activePlayer();

    p.startTurn(*this);

    std::cout << "[PLAYER TURN] " << p.name
        << " | HP: " << p.hp << "/" << p.maxHp
        << " | Mana: " << p.mana
        << " | Hand: " << p.hand.cards.size()
        << " | Deck: " << p.deck.cards.size()
        << std::endl;
}

void CardGameState::startEnemyTurn() {
    if (enemyTurnIndex < 0 ||
        enemyTurnIndex >= static_cast<int>(enemies.size())) {
        return;
    }
    CombatPlayer& enemy = enemies[enemyTurnIndex];

    enemy.cardsPlayedThisTurn = 0;

    if (enemy.hp <= 0) {
        std::cout << "[ENEMY TURN] " << enemy.name
            << " is defeated and skips their turn.\n";
        enemy.endTurn();
        triggerOnEndOfTurnTraps(enemy);
        enemyTurnPending = true;
        enemyTurnPauseTime = 3.0; // seconds
        return;
    }

    // Give enemy mana (+2 per turn)
    enemy.gainMana(2);

    if (enemy.stunned) {
        std::cout << "[ENEMY TURN] " << enemy.name
            << " is stunned and cannot act this turn.\n";
        enemy.endTurn();                // burn, auras tick, stun clears here
        enemyTurnPending = true;
        enemyTurnPauseTime = 3.0;
        return;
    }

    if (!enemy.deck.cards.empty()) {
        std::uniform_int_distribution<size_t> dist(0, enemy.deck.cards.size() - 1);
        size_t        cardIdx = dist(rng);
        CardInstance  card = enemy.deck.cards[cardIdx];

        // Remove from deck
        enemy.deck.cards.erase(enemy.deck.cards.begin() + cardIdx);

        // Determine targets based on card's TargetScope
        std::vector<CombatPlayer*> targets;
        if (card.def) {
            switch (card.def->targetScope) {
            case TargetScope::SingleEnemy:
                // Prefer a Waterfall-taunt player if any
                if (!players.empty()) {
                    CombatPlayer* tauntTarget = nullptr;
                    for (auto& p : players) {
                        if (p.hp > 0 && p.aura.kind == AuraKind::Waterfall) {
                            tauntTarget = &p;
                            break;
                        }
                    }

                    if (tauntTarget) {
                        targets.push_back(tauntTarget);
                    }
                    else {
                        std::uniform_int_distribution<size_t> pdist(0, players.size() - 1);
                        targets.push_back(&players[pdist(rng)]);
                    }
                }
                break;

            case TargetScope::SingleAlly:
            case TargetScope::Self:
                targets.push_back(&enemy);
                break;
            case TargetScope::Everyone:
                for (auto& p : players) {
                    targets.push_back(&p);
                }
                break;
            default:
                break;
            }
        }

        int cost = card.def ? card.def->manaCost : 0;
        if (enemy.mana >= cost) {

            // --- NEW: Snuff Out counterspell check (before paying mana) ---
            if (tryCounterWithSnuffOut(enemy, card)) {
                // Card is countered: goes to enemy's discard, no mana spent, no effect
                enemy.discard.addToTop(card);

                enemy.cardsPlayedThisTurn++;
                triggerOnCardPlayedTraps(enemy);
            }
            else {
                // Normal card play
                enemy.mana -= cost;

                std::cout << "[ENEMY TURN] " << enemy.name
                    << " | HP: " << enemy.hp << "/" << enemy.maxHp
                    << " | Mana: " << enemy.mana
                    << " | Played: " << (card.def ? card.def->name : "<unnamed>");
                if (!targets.empty()) {
                    std::cout << " on";
                    for (size_t i = 0; i < targets.size(); ++i) {
                        std::cout << (i == 0 ? " " : ", ") << targets[i]->name;
                    }
                }
                std::cout << "\n";

                if (card.def && card.def->type == CardType::Trap) {
                    addTrap(enemy, *card.def);
                    enemy.discard.addToTop(card);
                }
                else {
                    if (card.def && card.def->effect) {
                        EffectContext ctx{ *this, enemy, targets };
                        card.def->effect(ctx);
                    }
                    enemy.discard.addToTop(card);
                }

                enemy.cardsPlayedThisTurn++;
                triggerOnCardPlayedTraps(enemy);
            }
        }
        else {
            std::cout << "[ENEMY TURN] " << enemy.name
                << " has insufficient mana to play a card.\n";
            // Put card back on top of deck so they might use it next time
            enemy.deck.addToTop(card);
        }
    }
    else {
        std::cout << "[ENEMY TURN] " << enemy.name
            << " has no cards left in deck.\n";
        ++enemy.fatigueCounter;
        enemy.takeDamage(enemy.fatigueCounter);
        std::cout << "[FATIGUE] Enemy " << enemy.name
            << " suffers fatigue " << enemy.fatigueCounter << "\n";
    }

    enemy.endTurn();

    enemyTurnPending = true;
    enemyTurnPauseTime = 3.0; // seconds
}

void CardGameState::endTurn() {
    if (firstRound) {
        firstRoundTurnsRemaining--;
        if (firstRoundTurnsRemaining <= 0) {
            firstRound = false;
        }
    }

    // If we are in player turns
    if (enemyTurnIndex == -1) {
        // End turn for the current active player
        CombatPlayer& p = activePlayer();
        p.endTurn();
        triggerOnEndOfTurnTraps(p);

        if (!players.empty()) {
            activePlayerIndex =
                (activePlayerIndex + 1) % static_cast<int>(players.size());
        }

        // If we've looped back to player 0, start enemy turns
        if (activePlayerIndex == 0) {
            enemyTurnIndex = 0;
            if (!enemies.empty()) {
                startEnemyTurn();
            }
            else {
                // No enemies, just continue with next player turn
                startTurn();
            }
            return;
        }

        // Next player's turn
        startTurn();
    }
    // If we are in enemy turns
    else {
        // don't call activePlayer().endTurn() here; enemies handle their
        // own endTurn() inside startEnemyTurn().
        enemyTurnIndex++;
        if (enemyTurnIndex < static_cast<int>(enemies.size())) {
            startEnemyTurn();
        }
        else {
            // All enemies have acted, back to player turns
            enemyTurnIndex = -1;
            activePlayerIndex = 0;
            startTurn();
        }
    }
}



bool CardGameState::playCardFromHand(size_t handIndex,
    const std::vector<CombatPlayer*>& targets)
{
    CombatPlayer& p = activePlayer();
    if (p.stunned) {
        std::cout << "[PLAY] " << p.name
            << " is stunned and cannot play cards this turn.\n";
        return false;
    }
    if (handIndex >= p.hand.cards.size()) {
        return false; // invalid index
    }

    CardInstance card = p.hand.cards[handIndex];
    if (!card.def) {
        return false;
    }

    int cost = card.def->manaCost;
    if (p.mana < cost) {
        std::cout << "[PLAY] Not enough mana to play '"
            << card.def->name << "'\n";
        return false;
    }

    // Pay cost
    p.mana -= cost;

    if (tryCounterWithSnuffOut(p, card)) {
        // Remove card from hand (swap-with-back) and put into discard
        if (handIndex != p.hand.cards.size() - 1) {
            p.hand.cards[handIndex] = p.hand.cards.back();
        }
        p.hand.cards.pop_back();
        p.discard.addToTop(card);

        p.cardsPlayedThisTurn++;
        triggerOnCardPlayedTraps(p);
        return true; // card was 'played' but countered
    }

    // Remove from hand (swap-with-back)
    if (handIndex != p.hand.cards.size() - 1) {
        p.hand.cards[handIndex] = p.hand.cards.back();
    }
    p.hand.cards.pop_back();

    std::cout << "[PLAY] " << p.name << " plays '"
        << card.def->name << "' (cost " << cost
        << "), mana now " << p.mana << "\n";

    if (card.def->type == CardType::Trap) {
        addTrap(p, *card.def);
        // Physical card can go straight to discard
        p.discard.addToTop(card);

        p.cardsPlayedThisTurn++;
        triggerOnCardPlayedTraps(p);

        return true;
    }

    // Queue + immediate resolution for non-trap cards
    p.queue.enqueue(card, targets);
    resolveQueue();

    // Count card plays for Snake Trap etc.
    p.cardsPlayedThisTurn++;
    triggerOnCardPlayedTraps(p);

    // Aura hook: Rainy Presence triggers on every card played
    if (p.aura.kind == AuraKind::RainyPresence) {
        p.stats.attack += 1;
        p.stats.defense += 1;
        std::cout << "[AURA] Rainy Presence: " << p.name
            << " gains +1 attack and +1 block (now "
            << p.stats.attack << " strength, " << p.stats.defense << " block).\n";
    }

    return true;
}

void CardGameState::dealStartingHands(int numCards) {
    for (auto& player : players) {
        player.drawCards(numCards, rng);
    }
}

void CardGameState::conjureCardToHand(CombatPlayer& owner,
    const std::string& cardName,
    bool ephemeral)
{
    const CardDefinition* def = LizardCards::findByName(cardName);
    if (!def) {
        std::cout << "[CONJURE] Could not find card '" << cardName << "'.\n";
        return;
    }

    CardInstance inst;
    inst.def = def;
    inst.instanceId = nextInstanceIdGlobal++;
    inst.ephemeral = ephemeral;

    owner.hand.addToTop(inst);
    std::cout << "[CONJURE] " << owner.name << " conjures '" << cardName
        << "' into hand" << (ephemeral ? " (ephemeral)" : "") << ".\n";
}

void CardGameState::resolveQueue() {
    CombatPlayer& p = activePlayer();
    while (!p.queue.empty()) {
        QueuedCard qc = p.queue.popFront();
        if (!qc.card.def) {
            // unknown card, just discard
            p.discard.addToTop(qc.card);
            continue;
        }

        if (qc.card.def->effect) {
            EffectContext ctx{ *this, p, qc.targets };
            qc.card.def->effect(ctx);
        }

        // After resolution, card goes to discard UNLESS ephemeral
        if (!qc.card.ephemeral) {
            p.discard.addToTop(qc.card);
        }
        else {
            std::cout << "[EPHEMERAL] '" << qc.card.def->name
                << "' is ephemeral and is removed from the game.\n";
        }
    }
}

void CardGameState::addTrap(CombatPlayer& owner, const CardDefinition& def) {
    if (def.trapKind == TrapKind::None) return;

    TrapInstance t;
    t.kind = def.trapKind;
    t.def = &def;
    t.instanceId = nextTrapId++;

    owner.traps.push_back(t);
    std::cout << "[TRAP] " << owner.name << " arms '" << def.name << "'.\n";
}

void CardGameState::triggerOnAttackedTraps(CombatPlayer& attacker,
    CombatPlayer& victim,
    int damageDealt)
{
    auto& traps = victim.traps;
    for (auto it = traps.begin(); it != traps.end(); ) {
        if (it->kind == TrapKind::ExplosiveTrap) {
            std::cout << "[TRAP] Explosive Trap triggers on attack against "
                << victim.name << "!\n";

            attacker.takeDamage(5);
            attacker.stats.burn += 2;
            std::cout << "[TRAP] " << attacker.name
                << " takes 5 damage and gains 2 Burn (now "
                << attacker.stats.burn << ").\n";

            it = traps.erase(it); // one-shot trap
        }
        else if (it->kind == TrapKind::IceBlock) {
            // Reconstruct HP before the hit
            int hpBefore = victim.hp + damageDealt;

            // "Would die": was alive before, now at 0 or below, and damage was >0
            if (damageDealt > 0 && hpBefore > 0 && victim.hp <= 0) {
                victim.hp = 1;
                victim.stats.defense += 8;

                std::cout << "[TRAP] Ice Block triggers for " << victim.name
                    << "! Preventing lethal damage: HP set to 1 and +8 block "
                    << "(now " << victim.stats.defense << " block).\n";

                it = traps.erase(it); // consumed
            }
            else {
                ++it;
            }
        }
        else {
            ++it;
        }
    }
}


bool CardGameState::tryCounterWithSnuffOut(CombatPlayer& actor,
    const CardInstance& card)
{
    // Determine which side the actor is on
    bool actorIsPlayer = false;
    for (auto& p : players) {
        if (&p == &actor) {
            actorIsPlayer = true;
            break;
        }
    }

    auto& opponents = actorIsPlayer ? enemies : players;

    // Look for a Snuff Out trap on the opposing team
    for (CombatPlayer& owner : opponents) {
        auto& traps = owner.traps;
        for (auto it = traps.begin(); it != traps.end(); ++it) {
            if (it->kind == TrapKind::SnuffOut) {
                std::cout << "[TRAP] Snuff Out controlled by " << owner.name
                    << " counters '"
                    << (card.def ? card.def->name : "<unnamed>")
                    << "' played by " << actor.name << ".\n";

                // Consume this trap
                traps.erase(it);
                return true;
            }
        }
    }

    return false;
}

void CardGameState::onHeal(CombatPlayer& source,
    CombatPlayer& target,
    int amountHealed)
{
    if (amountHealed <= 0) return;

    // Deep Roots: "Whenever you heal a friend, that friend gains 2 strength"
    // "friend" -> same team, but NOT self
    if (source.aura.kind == AuraKind::DeepRoots) {
        // Determine if target is ally and not self
        bool sourceIsPlayer = false;
        for (auto& p : players) {
            if (&p == &source) {
                sourceIsPlayer = true;
                break;
            }
        }

        auto& allyTeam = sourceIsPlayer ? players : enemies;

        bool isAlly = false;
        for (auto& a : allyTeam) {
            if (&a == &target) {
                isAlly = true;
                break;
            }
        }

        if (isAlly && &target != &source) {
            target.stats.attack += 2;
            std::cout << "[AURA] Deep Roots: " << target.name
                << " gains +2 strength after being healed by "
                << source.name << " (now " << target.stats.attack
                << " strength).\n";
        }
    }
}

void CardGameState::triggerOnEndOfTurnTraps(CombatPlayer& actor)
{
    // Spike Trap only cares about turns where *no* cards were played.
    if (actor.cardsPlayedThisTurn > 0) {
        return;
    }

    // Determine which side the actor is on.
    bool actorIsPlayer = false;
    for (auto& p : players) {
        if (&p == &actor) {
            actorIsPlayer = true;
            break;
        }
    }

    auto& possibleOwners = actorIsPlayer ? enemies : players;

    for (CombatPlayer& owner : possibleOwners) {
        auto& traps = owner.traps;
        for (auto it = traps.begin(); it != traps.end(); ) {
            if (it->kind == TrapKind::SpikeTrap) {
                std::cout << "[TRAP] Spike Trap triggers! "
                    << actor.name
                    << " ended their turn without playing a card.\n";

                int beforeHp = actor.hp;
                actor.takeDamage(5);
                int dealt = beforeHp - actor.hp;

                std::cout << "[TRAP] " << actor.name
                    << " takes 5 damage from Spike Trap.\n";

                // Treat this as direct damage from the trap owner
                // (for Fire Spitter, etc.).
                onDirectDamage(owner, actor, dealt);

                it = traps.erase(it); // Spike Trap is one-shot
            }
            else {
                ++it;
            }
        }
    }
}


void CardGameState::triggerOnCardPlayedTraps(CombatPlayer& actor)
{
    if (actor.cardsPlayedThisTurn < 3) {
        return; // no trigger yet
    }

    // Determine which side the actor is on
    bool actorIsPlayer = false;
    for (auto& p : players) {
        if (&p == &actor) {
            actorIsPlayer = true;
            break;
        }
    }

    auto& possibleOwners = actorIsPlayer ? enemies : players;

    for (CombatPlayer& owner : possibleOwners) {
        auto& traps = owner.traps;
        for (auto it = traps.begin(); it != traps.end(); ) {
            if (it->kind == TrapKind::SnakeTrap) {
                std::cout << "[TRAP] Snake Trap triggers! "
                    << actor.name << " has played "
                    << actor.cardsPlayedThisTurn << " cards this turn.\n";

                actor.takeDamage(5);
                int oldHp = owner.hp;
                owner.hp = std::min(owner.hp + 5, owner.maxHp);
                std::cout << "[TRAP] " << actor.name
                    << " takes 5 damage; " << owner.name
                    << " heals " << (owner.hp - oldHp)
                    << " to " << owner.hp << "/" << owner.maxHp << "\n";

                it = traps.erase(it); // remove this trap
            }
            else {
                ++it;
            }
        }
    }
}




// ---------------- CARD DATABASE / HELPERS ----------------
// Mostly ordered by type, except beginning
namespace {

    std::vector<CardDefinition>& cardStorage() {
        static std::vector<CardDefinition> cards;
        return cards;
    }

    bool cardsInitialized = false;

    void initCards() {
        if (cardsInitialized) return;
        cardsInitialized = true;

        auto& cards = cardStorage();
        int nextId = 1;

        // --- FIREBALL ---
        {
            CardDefinition fireball;
            fireball.id = nextId++;
            fireball.name = "Fireball";
            fireball.type = CardType::Spell;
            fireball.school = School::Fire;
            fireball.manaCost = 2;
            fireball.targetScope = TargetScope::SingleEnemy;
            fireball.texturePath = "media/cards/fireball.png";

            fireball.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int baseDamage = 10;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                // Figure out which side is "enemies" relative to the source
                bool sourceIsPlayer = false;
                for (auto& p : ctx.state.players) {
                    if (&p == &ctx.source) {
                        sourceIsPlayer = true;
                        break;
                    }
                }

                auto& enemyTeam = sourceIsPlayer ? ctx.state.enemies
                    : ctx.state.players;

                for (auto& enemy : enemyTeam) {
                    enemy.stats.burn += 2;
                }

                std::cout << "[CARD] Fireball hits " << target->name
                    << " for " << totalDamage
                    << " and applies 2 Burn to all enemies.\n";

                // Central damage hook: Fire Spitter + traps, etc.
                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(fireball);
        }

        // --- KINDLE ---
        {
            CardDefinition kindle;
            kindle.id = nextId++;
            kindle.name = "Kindle";
            kindle.type = CardType::Spell;
            kindle.school = School::Fire;
            kindle.manaCost = 1;
            kindle.targetScope = TargetScope::SingleEnemy;
            kindle.texturePath = "media/cards/kindle.png"; // placeholder path

            kindle.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                target->stats.burn += 3;
                std::cout << "[CARD] Kindle applies 3 Burn to "
                    << target->name << " (now " << target->stats.burn << ").\n";
                };

            cards.push_back(kindle);
        }

        // --- DETONATE ---
        {
            CardDefinition detonate;
            detonate.id = nextId++;
            detonate.name = "Detonate";
            detonate.type = CardType::Spell;
            detonate.school = School::Fire;
            detonate.manaCost = 0;
            detonate.targetScope = TargetScope::SingleEnemy;
            detonate.texturePath = "media/cards/detonate.png"; // placeholder path

            detonate.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int n = target->stats.burn;
                if (n <= 0) {
                    std::cout << "[CARD] Detonate: " << target->name
                        << " has no burn; nothing happens.\n";
                    return;
                }

                // N + (N-1) + ... + 1 = N*(N+1)/2
                int totalDamage = n * (n + 1) / 2;
                target->stats.burn = 0;
                target->takeDamage(totalDamage);

                std::cout << "[CARD] Detonate consumes burn for "
                    << totalDamage << " damage on " << target->name << ".\n";
                };

            cards.push_back(detonate);
        }


        // --- EXPLOSIVE TRAP ---
        // Fire 1 Trap Enemy: When you are attacked deal 5 damage and apply 2 Burn
        // to the enemy attacking you.
        {
            CardDefinition trap;
            trap.id = nextId++;
            trap.name = "Explosive Trap";
            trap.type = CardType::Trap;
            trap.school = School::Fire;
            trap.manaCost = 1;
            trap.targetScope = TargetScope::Self; // you arm it on yourself
            trap.texturePath = "media/cards/explosive_trap.png"; // if you have one
            trap.trapKind = TrapKind::ExplosiveTrap;

            // No immediate effect when played; all behavior is in trap system
            trap.effect = nullptr;

            cards.push_back(trap);
        }

        // --- FIRE SPITTER ---
        // Aura, cost 0: Whenever you deal damage to an enemy, apply 1 Burn to that enemy.
        {
            CardDefinition spitter;
            spitter.id = nextId++;
            spitter.name = "Fire Spitter";
            spitter.type = CardType::Aura;
            spitter.school = School::Fire;
            spitter.manaCost = 0;
            spitter.targetScope = TargetScope::Self;
            spitter.texturePath = "media/cards/fire_spitter.png"; // placeholder path

            spitter.effect = [](const EffectContext& ctx) {
                ctx.state.applyAura(ctx.source, AuraKind::FireSpitter);
                };

            cards.push_back(spitter);
        }

        // --- SPARKS ---
        // Fire 1 Sorcery Enemy: Deal 5 damage to target enemy.
        {
            CardDefinition sparks;
            sparks.id = nextId++;
            sparks.name = "Sparks";
            sparks.type = CardType::Spell;
            sparks.school = School::Fire;
            sparks.manaCost = 1;
            sparks.targetScope = TargetScope::SingleEnemy;
            sparks.texturePath = "media/cards/sparks.png"; // placeholder

            sparks.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int baseDamage = 5;
                // For consistency with Fireball, include attack bonus:
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Sparks hits " << target->name
                    << " for " << totalDamage << ".\n";

                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(sparks);
        }

        // --- MAGMA ARMOR ---
        // Fire 1 Sorcery Self: Take 5 damage, gain 3 strength and 2 block.
        {
            CardDefinition magma;
            magma.id = nextId++;
            magma.name = "Magma Armor";
            magma.type = CardType::Spell;
            magma.school = School::Fire;
            magma.manaCost = 1;
            magma.targetScope = TargetScope::Self;
            magma.texturePath = "media/cards/magma_armor.png"; // placeholder

            magma.effect = [](const EffectContext& ctx) {
                int selfDamage = 5;

                int beforeHp = ctx.source.hp;
                ctx.source.takeDamage(selfDamage);
                int dealt = beforeHp - ctx.source.hp;

                ctx.source.stats.attack += 3;
                ctx.source.stats.defense += 2;

                std::cout << "[CARD] Magma Armor: " << ctx.source.name
                    << " takes " << dealt
                    << " damage and gains +3 strength and +2 block "
                    << "(now " << ctx.source.stats.attack << " strength, "
                    << ctx.source.stats.defense << " block).\n";
                };

            cards.push_back(magma);
        }

        // --- FUEL ---
        // Fire 2 Sorcery Enemy: Double target enemy's Burn.
        {
            CardDefinition fuel;
            fuel.id = nextId++;
            fuel.name = "Fuel";
            fuel.type = CardType::Spell;
            fuel.school = School::Fire;
            fuel.manaCost = 2;
            fuel.targetScope = TargetScope::SingleEnemy;
            fuel.texturePath = "media/cards/fuel.png"; // placeholder

            fuel.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int before = target->stats.burn;
                if (before <= 0) {
                    std::cout << "[CARD] Fuel: " << target->name
                        << " has no Burn; nothing to double.\n";
                    return;
                }
                target->stats.burn *= 2;
                std::cout << "[CARD] Fuel: " << target->name
                    << "'s Burn is doubled from " << before
                    << " to " << target->stats.burn << ".\n";
                };

            cards.push_back(fuel);
        }

        // --- WHEEL OF FLAMES ---
        // Fire 2 Sorcery Self: Discard your hand, draw 3 cards.
        {
            CardDefinition wheel;
            wheel.id = nextId++;
            wheel.name = "Wheel of Flames";
            wheel.type = CardType::Spell;
            wheel.school = School::Fire;
            wheel.manaCost = 2;
            wheel.targetScope = TargetScope::Self;
            wheel.texturePath = "media/cards/wheel_of_flames.png"; // placeholder

            wheel.effect = [](const EffectContext& ctx) {
                auto& hand = ctx.source.hand.cards;
                auto& discard = ctx.source.discard.cards;

                int discarded = static_cast<int>(hand.size());
                for (auto& c : hand) {
                    discard.push_back(c);
                }
                hand.clear();

                std::cout << "[CARD] Wheel of Flames: " << ctx.source.name
                    << " discards " << discarded << " card(s) and draws 3.\n";

                ctx.source.drawCards(3, ctx.state.rng);
                };

            cards.push_back(wheel);
        }

        // --- VOLCANIC ERUPTION ---
        // Fire 3 Sorcery Enemy: Deal 16 damage to target enemy, set that enemy's mana to 0.
        {
            CardDefinition volc;
            volc.id = nextId++;
            volc.name = "Volcanic Eruption";
            volc.type = CardType::Spell;
            volc.school = School::Fire;
            volc.manaCost = 3;
            volc.targetScope = TargetScope::SingleEnemy;
            volc.texturePath = "media/cards/volcanic_eruption.png"; // placeholder

            volc.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int baseDamage = 16;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                target->mana = 0;

                std::cout << "[CARD] Volcanic Eruption hits " << target->name
                    << " for " << totalDamage
                    << " and sets their mana to 0.\n";

                // Fire Spitter, traps, etc.
                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(volc);
        }

        // --- METEOR STORM ---
        // Fire 3 Sorcery All Enemies: Deal 12 damage to all enemies.
        {
            CardDefinition meteor;
            meteor.id = nextId++;
            meteor.name = "Meteor Storm";
            meteor.type = CardType::Spell;
            meteor.school = School::Fire;
            meteor.manaCost = 3;
            meteor.targetScope = TargetScope::Self; // cast on yourself / play zone, effect hits all enemies
            meteor.texturePath = "media/cards/meteor_swarm.png"; // placeholder

            meteor.effect = [](const EffectContext& ctx) {
                // Determine enemy team relative to source
                bool sourceIsPlayer = false;
                for (auto& p : ctx.state.players) {
                    if (&p == &ctx.source) {
                        sourceIsPlayer = true;
                        break;
                    }
                }
                auto& enemyTeam = sourceIsPlayer ? ctx.state.enemies
                    : ctx.state.players;

                if (enemyTeam.empty()) {
                    std::cout << "[CARD] Meteor Storm: no enemies to hit.\n";
                    return;
                }

                int baseDamage = 12;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                std::cout << "[CARD] Meteor Storm hits all enemies for "
                    << totalDamage << " damage.\n";

                for (auto& enemy : enemyTeam) {
                    if (enemy.isDead()) continue;

                    int beforeHp = enemy.hp;
                    enemy.takeDamage(totalDamage);
                    int dealt = beforeHp - enemy.hp;

                    ctx.state.onDirectDamage(ctx.source, enemy, dealt);
                }
                };

            cards.push_back(meteor);
        }

        // --- DRAGON'S BREATH ---
        // Fire X Sorcery All Enemies:
        // X = current mana. Apply X Burn to all enemies and consume all mana.
        {
            CardDefinition breath;
            breath.id = nextId++;
            breath.name = "Dragon's Breath";
            breath.type = CardType::Spell;
            breath.school = School::Fire;
            breath.manaCost = 0; // X is handled in the effect by consuming all mana
            breath.targetScope = TargetScope::Self; // cast on yourself / play zone
            breath.texturePath = "media/cards/dragons_breath.png"; // placeholder

            breath.effect = [](const EffectContext& ctx) {
                // Determine enemy team relative to source
                bool sourceIsPlayer = false;
                for (auto& p : ctx.state.players) {
                    if (&p == &ctx.source) {
                        sourceIsPlayer = true;
                        break;
                    }
                }
                auto& enemyTeam = sourceIsPlayer ? ctx.state.enemies
                    : ctx.state.players;

                int x = ctx.source.mana; // X is current mana at resolution
                if (x <= 0 || enemyTeam.empty()) {
                    std::cout << "[CARD] Dragon's Breath: no mana or no enemies; nothing happens.\n";
                    return;
                }

                std::cout << "[CARD] Dragon's Breath: " << ctx.source.name
                    << " spends " << x << " mana to apply "
                    << x << " Burn to all enemies.\n";

                ctx.source.mana = 0;

                for (auto& enemy : enemyTeam) {
                    if (enemy.isDead()) continue;
                    enemy.stats.burn += x;
                    std::cout << "    -> " << enemy.name << " now has "
                        << enemy.stats.burn << " Burn.\n";
                }
                };

            cards.push_back(breath);
        }

        // --- HYDRATE ---
        {
            CardDefinition hydrate;
            hydrate.id = nextId++;
            hydrate.name = "Hydrate";
            hydrate.type = CardType::Spell;
            hydrate.school = School::Water;
            hydrate.manaCost = 0;
            hydrate.targetScope = TargetScope::Self;
            hydrate.texturePath = "media/cards/hydrate.png"; // placeholder

            hydrate.effect = [](const EffectContext& ctx) {
                ctx.source.gainMana(1);
                std::cout << "[CARD] Hydrate: " << ctx.source.name
                    << " gains 1 mana (now " << ctx.source.mana << ").\n";
                };

            cards.push_back(hydrate);
        }

        // --- DIVE ---
        {
            CardDefinition dive;
            dive.id = nextId++;
            dive.name = "Dive";
            dive.type = CardType::Spell;
            dive.school = School::Water;
            dive.manaCost = 0;
            dive.targetScope = TargetScope::Self;
            dive.texturePath = "media/cards/dive.png"; // placeholder

            dive.effect = [](const EffectContext& ctx) {
                ctx.source.stats.defense += 1;
                std::cout << "[CARD] Dive: " << ctx.source.name
                    << " gains 1 block (now " << ctx.source.stats.defense << ").\n";
                ctx.source.drawCards(1, ctx.state.rng);
                };

            cards.push_back(dive);
        }

        // --- ICE SHIVS ---
        // Water 1 Sorcery Self:
        // Conjure 3 Icicle cards to your hand (ephemeral).
        {
            CardDefinition shivs;
            shivs.id = nextId++;
            shivs.name = "Ice Shivs";
            shivs.type = CardType::Spell;
            shivs.school = School::Water;
            shivs.manaCost = 1;
            shivs.targetScope = TargetScope::Self;
            shivs.texturePath = "media/cards/ice_shivs.png"; // placeholder

            shivs.effect = [](const EffectContext& ctx) {
                std::cout << "[CARD] Ice Shivs: " << ctx.source.name
                    << " conjures 3 Icicles.\n";
                for (int i = 0; i < 3; ++i) {
                    ctx.state.conjureCardToHand(ctx.source, "Icicle", true);
                }
                };

            cards.push_back(shivs);
        }


        // --- ICICLE ---
        // Water 0 Sorcery Enemy: Deal 1 damage + attack to target enemy.
        // Typically conjured and ephemeral (removed from game when played).
        {
            CardDefinition icicle;
            icicle.id = nextId++;
            icicle.name = "Icicle";
            icicle.type = CardType::Spell;
            icicle.school = School::Water;
            icicle.manaCost = 0;
            icicle.targetScope = TargetScope::SingleEnemy;
            icicle.texturePath = "media/cards/icicle.png"; // placeholder

            icicle.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int baseDamage = 1;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Icicle: " << target->name
                    << " takes " << totalDamage << " damage.\n";

                // Fire Spitter, traps, etc.
                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(icicle);
        }

        // --- SPLASH ---
        // Water 1 Sorcery Enemy:
        // Deal damage to target enemy equal to your block, then lose all block.
        // Stun that enemy for 1 turn.
        {
            CardDefinition splash;
            splash.id = nextId++;
            splash.name = "Splash";
            splash.type = CardType::Spell;
            splash.school = School::Water;
            splash.manaCost = 1;
            splash.targetScope = TargetScope::SingleEnemy;
            splash.texturePath = "media/cards/splash.png"; // placeholder

            splash.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int blockValue = ctx.source.stats.defense;
                int baseDamage = blockValue;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Splash: " << ctx.source.name
                    << " uses " << blockValue << " block and "
                    << ctx.source.stats.attack << " attack to deal "
                    << totalDamage << " damage to " << target->name << ".\n";

                // Lose all block
                ctx.source.stats.defense = 0;
                std::cout << "[CARD] Splash: " << ctx.source.name
                    << " loses all block.\n";

                // Stun target for 1 turn
                if (target->stunned || target->stunProtected) {
                    std::cout << "[STUN] " << target->name
                        << " resists the stun (already stunned or stunned last turn).\n";
                }
                else {
                    target->stunned = true;
                    std::cout << "[STUN] " << target->name
                        << " is stunned and cannot play cards on their next turn.\n";
                }

                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(splash);
        }

        // --- CLEAR WATER (draw 2) ---
        {
            CardDefinition wave;
            wave.id = nextId++;
            wave.name = "Clear Water";
            wave.type = CardType::Spell;
            wave.school = School::Water;
            wave.manaCost = 1;
            wave.targetScope = TargetScope::Self;
            wave.texturePath = "media/cards/clear_water.png";

            wave.effect = [](const EffectContext& ctx) {
                std::cout << "[CARD] Clear Water: " << ctx.source.name
                    << " draws 2 cards.\n";
                ctx.source.drawCards(2, ctx.state.rng);
                };

            cards.push_back(wave);
        }

        // --- ICE BLOCK ---
        // Water 1 Trap Self: When you are attacked and would die, gain 8 block.
        {
            CardDefinition iceBlock;
            iceBlock.id = nextId++;
            iceBlock.name = "Ice Block";
            iceBlock.type = CardType::Trap;
            iceBlock.school = School::Water;
            iceBlock.manaCost = 1;
            iceBlock.targetScope = TargetScope::Self;  // you arm it on yourself
            iceBlock.texturePath = "media/cards/ice_block.png"; // placeholder
            iceBlock.trapKind = TrapKind::IceBlock;

            iceBlock.effect = nullptr; // behavior handled via trap system

            cards.push_back(iceBlock);
        }

        // --- RAINY PRESENCE ---
        // Aura, cost 2: Whenever you play a card gain 1 strength and 1 block.
        {
            CardDefinition rainy;
            rainy.id = nextId++;
            rainy.name = "Rainy Presence";
            rainy.type = CardType::Aura;
            rainy.school = School::Water;
            rainy.manaCost = 2;
            rainy.targetScope = TargetScope::Self;
            rainy.texturePath = "media/cards/rainy_presence.png"; // placeholder

            rainy.effect = [](const EffectContext& ctx) {
                ctx.state.applyAura(ctx.source, AuraKind::RainyPresence);
                };

            cards.push_back(rainy);
        }

        // --- WASH AWAY ---
        // Water 2 Sorcery Friend:
        // Remove all buffs, auras, and burn from target friend.
        {
            CardDefinition wash;
            wash.id = nextId++;
            wash.name = "Wash Away";
            wash.type = CardType::Spell;
            wash.school = School::Water;
            wash.manaCost = 2;
            wash.targetScope = TargetScope::SingleAlly;
            wash.texturePath = "media/cards/wash_away.png"; // placeholder

            wash.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int oldAttack = target->stats.attack;
                int oldDefense = target->stats.defense;
                int oldHealBonus = target->stats.healBonus;
                int oldBurn = target->stats.burn;
                AuraKind oldAura = target->aura.kind;

                target->stats.attack = 0;
                target->stats.defense = 0;
                target->stats.healBonus = 0;
                target->stats.burn = 0;
                target->aura.kind = AuraKind::None;
                target->aura.remainingTurns = 0;

                std::cout << "[CARD] Wash Away: " << target->name
                    << " is cleansed (removed "
                    << "attack " << oldAttack
                    << ", block " << oldDefense
                    << ", healBonus " << oldHealBonus
                    << ", burn " << oldBurn
                    << ", aura "
                    << (oldAura == AuraKind::None ? "None" : "Active")
                    << ").\n";
                };

            cards.push_back(wash);
        }

        // --- BRAINSTORM ---
        // Water 2 Sorcery Self:
        // Draw 5 cards, then discard 2 cards at random.
        {
            CardDefinition brain;
            brain.id = nextId++;
            brain.name = "Brainstorm";
            brain.type = CardType::Spell;
            brain.school = School::Water;
            brain.manaCost = 2;
            brain.targetScope = TargetScope::Self;
            brain.texturePath = "media/cards/brainstorm.png"; // placeholder

            brain.effect = [](const EffectContext& ctx) {
                CombatPlayer& src = ctx.source;

                std::cout << "[CARD] Brainstorm: " << src.name
                    << " draws 5 cards.\n";
                src.drawCards(5, ctx.state.rng);

                auto& hand = src.hand.cards;
                auto& discard = src.discard.cards;

                int toDiscard = std::min(2, static_cast<int>(hand.size()));
                if (toDiscard <= 0) {
                    std::cout << "[CARD] Brainstorm: no cards to discard.\n";
                    return;
                }

                std::cout << "[CARD] Brainstorm: discarding "
                    << toDiscard << " card(s) at random.\n";

                for (int i = 0; i < toDiscard; ++i) {
                    if (hand.empty()) break;

                    std::uniform_int_distribution<size_t> dist(0, hand.size() - 1);
                    size_t idx = dist(ctx.state.rng);

                    CardInstance c = hand[idx];
                    // Remove from hand (swap-with-back pattern)
                    if (idx != hand.size() - 1) {
                        hand[idx] = hand.back();
                    }
                    hand.pop_back();

                    discard.push_back(c);

                    std::cout << "    -> Discarded '"
                        << (c.def ? c.def->name : "<unnamed>")
                        << "'.\n";
                }
                };

            cards.push_back(brain);
        }

        // --- TSUNAMI ---
        // Water 3 Sorcery Enemy:
        // Deal 10 damage to target enemy. Stun all enemies for 1 turn.
        {
            CardDefinition tsunami;
            tsunami.id = nextId++;
            tsunami.name = "Tsunami";
            tsunami.type = CardType::Spell;
            tsunami.school = School::Water;
            tsunami.manaCost = 3;
            tsunami.targetScope = TargetScope::SingleEnemy;
            tsunami.texturePath = "media/cards/tsunami.png"; // placeholder

            tsunami.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* primary = ctx.targets[0];

                // Determine which side is "enemies" relative to source
                bool sourceIsPlayer = false;
                for (auto& p : ctx.state.players) {
                    if (&p == &ctx.source) {
                        sourceIsPlayer = true;
                        break;
                    }
                }
                auto& enemyTeam = sourceIsPlayer ? ctx.state.enemies
                    : ctx.state.players;

                // 1) Direct damage to the primary target
                int baseDamage = 10;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = primary->hp;
                primary->takeDamage(totalDamage);
                int dealt = beforeHp - primary->hp;

                std::cout << "[CARD] Tsunami hits " << primary->name
                    << " for " << totalDamage << " damage.\n";

                // central hook: Fire Spitter, traps, etc.
                ctx.state.onDirectDamage(ctx.source, *primary, dealt);

                // 2) Stun all enemies for 1 turn (respect stun protection)
                std::cout << "[CARD] Tsunami: stunning all enemies.\n";
                for (auto& enemy : enemyTeam) {
                    if (enemy.isDead()) continue;

                    if (enemy.stunned || enemy.stunProtected) {
                        std::cout << "    -> " << enemy.name
                            << " resists stun (already stunned or stunned last turn).\n";
                    }
                    else {
                        enemy.stunned = true;
                        std::cout << "    -> " << enemy.name
                            << " is stunned and cannot play cards on their next turn.\n";
                    }
                }
                };

            cards.push_back(tsunami);
        }

        // --- WATERFALL ---
        // Water 3 Aura Self:
        // Enemies target you with attacks. Whenever you are attacked, conjure 1 Icicle to your hand.
        {
            CardDefinition waterfall;
            waterfall.id = nextId++;
            waterfall.name = "Waterfall";
            waterfall.type = CardType::Aura;
            waterfall.school = School::Water;
            waterfall.manaCost = 3;
            waterfall.targetScope = TargetScope::Self;
            waterfall.texturePath = "media/cards/waterfall.png"; // placeholder

            waterfall.effect = [](const EffectContext& ctx) {
                ctx.state.applyAura(ctx.source, AuraKind::Waterfall);
                };

            cards.push_back(waterfall);
        }

        // --- FROSTWALKING ---
        // Water 3 Sorcery All Friends:
        // All allies gain 1 block for each card in your discard pile.
        {
            CardDefinition frost;
            frost.id = nextId++;
            frost.name = "Frostwalking";
            frost.type = CardType::Spell;
            frost.school = School::Water;
            frost.manaCost = 3;
            frost.targetScope = TargetScope::Self; // play on yourself, effect hits all allies
            frost.texturePath = "media/cards/frostwalking.png"; // placeholder

            frost.effect = [](const EffectContext& ctx) {
                CombatPlayer& src = ctx.source;

                // How many cards are in *your* discard pile?
                int discardCount = static_cast<int>(src.discard.cards.size());
                if (discardCount <= 0) {
                    std::cout << "[CARD] Frostwalking: " << src.name
                        << " has no cards in discard; no block gained.\n";
                    return;
                }

                // Determine ally team relative to source
                bool sourceIsPlayer = false;
                for (auto& p : ctx.state.players) {
                    if (&p == &src) {
                        sourceIsPlayer = true;
                        break;
                    }
                }
                auto& allyTeam = sourceIsPlayer ? ctx.state.players
                    : ctx.state.enemies;

                std::cout << "[CARD] Frostwalking: " << src.name
                    << " gives all allies +" << discardCount
                    << " block based on discard size (" << discardCount
                    << " cards).\n";

                for (auto& ally : allyTeam) {
                    if (ally.isDead()) continue;
                    ally.stats.defense += discardCount;
                    std::cout << "    -> " << ally.name << " now has "
                        << ally.stats.defense << " block.\n";
                }
                };

            cards.push_back(frost);
        }

        // --- GREEN LOTUS ---
        // Cost 0: Gain 3 heal bonus.
        {
            CardDefinition lotus;
            lotus.id = nextId++;
            lotus.name = "Green Lotus";
            lotus.type = CardType::Spell;
            lotus.school = School::Plant;
            lotus.manaCost = 0;
            lotus.targetScope = TargetScope::Self;
            lotus.texturePath = "media/cards/green_lotus.png"; // placeholder

            lotus.effect = [](const EffectContext& ctx) {
                ctx.source.stats.healBonus += 3;
                std::cout << "[CARD] Green Lotus: " << ctx.source.name
                    << " gains +3 heal bonus (now "
                    << ctx.source.stats.healBonus << ").\n";
                };

            cards.push_back(lotus);
        }

        // --- VINE WHIP ---
        // Deal 1 + attack to target enemy, then heal (1 + healBonus) to self
        {
            CardDefinition vine;
            vine.id = nextId++;
            vine.name = "Vine Whip";
            vine.type = CardType::Spell;
            vine.school = School::Plant;
            vine.manaCost = 0;
            vine.targetScope = TargetScope::SingleEnemy;
            vine.texturePath = "media/cards/vine_whip.png";

            vine.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int baseDamage = 1;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Vine Whip: " << target->name
                    << " takes " << totalDamage << " damage.\n";

                int baseHeal = 1;
                int totalHeal = baseHeal + ctx.source.stats.healBonus;
                if (totalHeal < 0) totalHeal = 0;

                int oldHp = ctx.source.hp;
                ctx.source.hp = std::min(ctx.source.hp + totalHeal, ctx.source.maxHp);
                int healed = ctx.source.hp - oldHp;

                std::cout << "[CARD] Vine Whip: " << ctx.source.name
                    << " heals " << healed
                    << " (base " << baseHeal
                    << " + healBonus " << ctx.source.stats.healBonus
                    << ") to " << ctx.source.hp << "/"
                    << ctx.source.maxHp << ".\n";

                ctx.state.onHeal(ctx.source, ctx.source, healed);
                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(vine);
        }



        // --- SNAKE TRAP ---
        // Plant 1 Trap Enemy: When an enemy plays 3 or more cards in a single turn,
        // deal 5 damage to them and heal 5 life.
        {
            CardDefinition trap;
            trap.id = nextId++;
            trap.name = "Snake Trap";
            trap.type = CardType::Trap;
            trap.school = School::Plant;
            trap.manaCost = 1;
            trap.targetScope = TargetScope::Self; // you arm it on yourself/your side
            trap.texturePath = "media/cards/snake_trap.png"; // if you have one
            trap.trapKind = TrapKind::SnakeTrap;

            trap.effect = nullptr; // armed via CardGameState::addTrap

            cards.push_back(trap);
        }

        // --- SNUFF OUT ---
        // Plant 1 Trap Enemy:
        // When an enemy would play a card, they discard that card instead.
        {
            CardDefinition snuff;
            snuff.id = nextId++;
            snuff.name = "Snuff Out";
            snuff.type = CardType::Trap;
            snuff.school = School::Plant;
            snuff.manaCost = 1;
            snuff.targetScope = TargetScope::Self; // arm it on yourself/your team
            snuff.texturePath = "media/cards/snuff_out.png"; // placeholder
            snuff.trapKind = TrapKind::SnuffOut;

            snuff.effect = nullptr; // behavior handled via tryCounterWithSnuffOut
            cards.push_back(snuff);
        }

        // --- BLOSSOM ---
        // Plant 1 Sorcery All Friends:
        // All friends gain 1 strength, 1 block, and 1 heal bonus
        // for each trap you currently control.
        {
            CardDefinition blossom;
            blossom.id = nextId++;
            blossom.name = "Blossom";
            blossom.type = CardType::Spell;
            blossom.school = School::Plant;
            blossom.manaCost = 1;
            blossom.targetScope = TargetScope::Self; // play on yourself, effect hits all allies
            blossom.texturePath = "media/cards/blossom.png"; // placeholder

            blossom.effect = [](const EffectContext& ctx) {
                CombatPlayer& src = ctx.source;

                int trapCount = static_cast<int>(src.traps.size());
                if (trapCount <= 0) {
                    std::cout << "[CARD] Blossom: " << src.name
                        << " controls no traps; no buffs granted.\n";
                    return;
                }

                // Determine allies relative to source
                bool sourceIsPlayer = false;
                for (auto& p : ctx.state.players) {
                    if (&p == &src) {
                        sourceIsPlayer = true;
                        break;
                    }
                }
                auto& allies = sourceIsPlayer ? ctx.state.players
                    : ctx.state.enemies;

                std::cout << "[CARD] Blossom: " << src.name
                    << " gives all allies +"
                    << trapCount << " attack, +"
                    << trapCount << " block, and +"
                    << trapCount << " heal bonus ("
                    << trapCount << " traps controlled).\n";

                for (auto& ally : allies) {
                    if (ally.isDead()) continue;

                    ally.stats.attack += trapCount;
                    ally.stats.defense += trapCount;
                    ally.stats.healBonus += trapCount;

                    std::cout << "    -> " << ally.name
                        << " now has strength " << ally.stats.attack
                        << ", block " << ally.stats.defense
                        << ", healBonus " << ally.stats.healBonus << ".\n";
                }
                };

            cards.push_back(blossom);
        }

        // --- ALOE VERA ---
        {
            CardDefinition aloe;
            aloe.id = nextId++;
            aloe.name = "Aloe Vera";
            aloe.type = CardType::Spell;
            aloe.school = School::Plant;
            aloe.manaCost = 1;
            aloe.targetScope = TargetScope::SingleAlly;
            aloe.texturePath = "media/cards/aloe_vera.png";

            aloe.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int oldBurn = target->stats.burn;
                target->stats.burn = 0;

                int baseHeal = 3;
                int totalHeal = baseHeal + ctx.source.stats.healBonus;
                if (totalHeal < 0) totalHeal = 0;

                int oldHp = target->hp;
                target->hp = std::min(target->hp + totalHeal, target->maxHp);
                int healed = target->hp - oldHp;

                std::cout << "[CARD] Aloe Vera: " << target->name
                    << " loses " << oldBurn << " Burn and heals "
                    << healed << " (base " << baseHeal
                    << " + healBonus " << ctx.source.stats.healBonus
                    << ") to " << target->hp << "/" << target->maxHp << ".\n";

                ctx.state.onHeal(ctx.source, *target, healed);
                };

            cards.push_back(aloe);
        }


        // --- VIGOR ---
        // Plant 2 Sorcery Friend:
        // Target friend heals 4 life and gains 3 strength.
        {
            CardDefinition vigor;
            vigor.id = nextId++;
            vigor.name = "Vigor";
            vigor.type = CardType::Spell;
            vigor.school = School::Plant;
            vigor.manaCost = 2;
            vigor.targetScope = TargetScope::SingleAlly;
            vigor.texturePath = "media/cards/vigor.png"; // placeholder

            vigor.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int baseHeal = 4;
                int totalHeal = baseHeal + ctx.source.stats.healBonus;
                if (totalHeal < 0) totalHeal = 0;

                int oldHp = target->hp;
                target->hp = std::min(target->hp + totalHeal, target->maxHp);
                int healed = target->hp - oldHp;

                target->stats.attack += 3;

                std::cout << "[CARD] Vigor: " << target->name
                    << " heals " << healed
                    << " (base " << baseHeal << " + healBonus "
                    << ctx.source.stats.healBonus << ") to "
                    << target->hp << "/" << target->maxHp
                    << " and gains +3 strength (now "
                    << target->stats.attack << " strength).\n";

                ctx.state.onHeal(ctx.source, *target, healed);
                };

            cards.push_back(vigor);
        }

        // --- DEEP ROOTS ---
        // Plant 2 Aura Self:
        // Whenever you heal a friend, that friend gains 2 strength.
        {
            CardDefinition roots;
            roots.id = nextId++;
            roots.name = "Deep Roots";
            roots.type = CardType::Aura;
            roots.school = School::Plant;
            roots.manaCost = 2;
            roots.targetScope = TargetScope::Self;
            roots.texturePath = "media/cards/deep_roots.png"; // placeholder

            roots.effect = [](const EffectContext& ctx) {
                ctx.state.applyAura(ctx.source, AuraKind::DeepRoots);
                };

            cards.push_back(roots);
        }


        // --- REGROW ---
        // Plant 2 Sorcery Self:
        // Return the top card from the discard pile to your hand.
        {
            CardDefinition regrow;
            regrow.id = nextId++;
            regrow.name = "Regrow";
            regrow.type = CardType::Spell;
            regrow.school = School::Plant;
            regrow.manaCost = 2;
            regrow.targetScope = TargetScope::Self;
            regrow.texturePath = "media/cards/regrow.png"; // placeholder

            regrow.effect = [](const EffectContext& ctx) {
                auto& discard = ctx.source.discard.cards;
                if (discard.empty()) {
                    std::cout << "[CARD] Regrow: discard pile is empty, nothing to return.\n";
                    return;
                }

                CardInstance inst = discard.back();
                discard.pop_back();
                ctx.source.hand.addToTop(inst);

                std::cout << "[CARD] Regrow: " << ctx.source.name
                    << " returns '" << (inst.def ? inst.def->name : "<unnamed>")
                    << "' from discard to hand.\n";
                };

            cards.push_back(regrow);
        }

        // --- VAMPIRISM ---
        // Plant 3 Sorcery Self:
        // All cards in your hand are replaced with Drain (ephemeral).
        {
            CardDefinition vamp;
            vamp.id = nextId++;
            vamp.name = "Vampirism";
            vamp.type = CardType::Spell;
            vamp.school = School::Plant;
            vamp.manaCost = 3;
            vamp.targetScope = TargetScope::Self;
            vamp.texturePath = "media/cards/vampirism.png"; // placeholder

            vamp.effect = [](const EffectContext& ctx) {
                const CardDefinition* drainDef = LizardCards::findByName("Drain");
                if (!drainDef) {
                    std::cout << "[CARD] Vampirism: 'Drain' card definition not found.\n";
                    return;
                }

                auto& hand = ctx.source.hand.cards;
                int transformed = 0;

                for (auto& inst : hand) {
                    inst.def = drainDef;
                    inst.ephemeral = true;   // removed from game when played
                    transformed++;
                }

                std::cout << "[CARD] Vampirism: " << ctx.source.name
                    << " transforms " << transformed
                    << " card(s) in hand into ephemeral Drain spells.\n";
                };

            cards.push_back(vamp);
        }


        // --- DRAIN ---
        // Plant 1 Sorcery Enemy:
        // Deal 3 + attack damage to target enemy, heal 3 + healBonus, draw 1.
        // Created by Vampirism and marked ephemeral. SHOULD NOT BE ADDED TO DECKS OR CARD LISTS
        {
            CardDefinition drain;
            drain.id = nextId++;
            drain.name = "Drain";
            drain.type = CardType::Spell;
            drain.school = School::Plant;
            drain.manaCost = 1;
            drain.targetScope = TargetScope::SingleEnemy;
            drain.texturePath = "media/cards/drain.png"; // placeholder

            drain.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                // Damage portion
                int baseDamage = 3;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Drain: " << target->name
                    << " takes " << totalDamage << " damage.\n";

                ctx.state.onDirectDamage(ctx.source, *target, dealt);

                // Heal portion
                int baseHeal = 3;
                int totalHeal = baseHeal + ctx.source.stats.healBonus;
                if (totalHeal < 0) totalHeal = 0;

                int oldHp = ctx.source.hp;
                ctx.source.hp = std::min(ctx.source.hp + totalHeal, ctx.source.maxHp);
                int healed = ctx.source.hp - oldHp;

                std::cout << "[CARD] Drain: " << ctx.source.name
                    << " heals " << healed
                    << " (base " << baseHeal
                    << " + healBonus " << ctx.source.stats.healBonus
                    << ") to " << ctx.source.hp << "/"
                    << ctx.source.maxHp << ".\n";

                ctx.state.onHeal(ctx.source, ctx.source, healed);

                // Draw 1 card
                ctx.source.drawCards(1, ctx.state.rng);
                };

            cards.push_back(drain);
        }

        // --- BOUNTIFUL HARVEST ---
        // Plant 3 Sorcery All Friends:
        // All friends heal 9 + healBonus and draw 1 card.
        {
            CardDefinition harvest;
            harvest.id = nextId++;
            harvest.name = "Bountiful Harvest";
            harvest.type = CardType::Spell;
            harvest.school = School::Plant;
            harvest.manaCost = 3;
            harvest.targetScope = TargetScope::Self;
            harvest.texturePath = "media/cards/bountiful_harvest.png"; // placeholder

            harvest.effect = [](const EffectContext& ctx) {
                // Determine allies relative to source
                bool sourceIsPlayer = false;
                for (auto& p : ctx.state.players) {
                    if (&p == &ctx.source) {
                        sourceIsPlayer = true;
                        break;
                    }
                }
                auto& allies = sourceIsPlayer ? ctx.state.players
                    : ctx.state.enemies;

                int baseHeal = 9;
                int totalHeal = baseHeal + ctx.source.stats.healBonus;
                if (totalHeal < 0) totalHeal = 0;

                std::cout << "[CARD] Bountiful Harvest: healing all allies for "
                    << totalHeal << " and drawing 1 card each.\n";

                for (auto& ally : allies) {
                    if (ally.isDead()) continue;

                    int oldHp = ally.hp;
                    ally.hp = std::min(ally.hp + totalHeal, ally.maxHp);
                    int healed = ally.hp - oldHp;

                    std::cout << "    -> " << ally.name << " heals " << healed
                        << " to " << ally.hp << "/" << ally.maxHp << "\n";

                    ctx.state.onHeal(ctx.source, ally, healed);
                    ally.drawCards(1, ctx.state.rng);
                }
                };

            cards.push_back(harvest);
        }

        // --- NATURE'S WRATH ---
        // Plant 3 Sorcery Enemy:
        // Convert your heal bonus to strength, then deal 12 + attack damage to target enemy.
        {
            CardDefinition wrath;
            wrath.id = nextId++;
            wrath.name = "Nature's Wrath";
            wrath.type = CardType::Spell;
            wrath.school = School::Plant;
            wrath.manaCost = 3;
            wrath.targetScope = TargetScope::SingleEnemy;
            wrath.texturePath = "media/cards/natures_wrath.png"; // placeholder

            wrath.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int converted = ctx.source.stats.healBonus;
                if (converted != 0) {
                    ctx.source.stats.healBonus = 0;
                    ctx.source.stats.attack += converted;

                    std::cout << "[CARD] Nature's Wrath: " << ctx.source.name
                        << " converts heal bonus " << converted
                        << " into strength (now " << ctx.source.stats.attack
                        << " strength, healBonus " << ctx.source.stats.healBonus
                        << ").\n";
                }
                else {
                    std::cout << "[CARD] Nature's Wrath: no heal bonus to convert.\n";
                }

                int baseDamage = 12;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Nature's Wrath hits " << target->name
                    << " for " << totalDamage << " damage.\n";

                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(wrath);
        }

        // --- SCRATCH ---
        // Basic 0 Sorcery Enemy:
        // Deal 1 + attack damage to target enemy.
        {
            CardDefinition scratch;
            scratch.id = nextId++;
            scratch.name = "Scratch";
            scratch.type = CardType::Spell;
            scratch.school = School::Generic; // "Basic"
            scratch.manaCost = 0;
            scratch.targetScope = TargetScope::SingleEnemy;
            scratch.texturePath = "media/cards/scratch.png"; // placeholder

            scratch.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int baseDamage = 1;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Scratch: " << target->name
                    << " takes " << totalDamage << " damage.\n";

                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(scratch);
        }

        // --- PIERCING BITE ---
        // Basic 1 Sorcery Enemy:
        // Target enemy loses all block, then take 3 + attack damage.
        {
            CardDefinition pbite;
            pbite.id = nextId++;
            pbite.name = "Piercing Bite";
            pbite.type = CardType::Spell;
            pbite.school = School::Generic;
            pbite.manaCost = 1;
            pbite.targetScope = TargetScope::SingleEnemy;
            pbite.texturePath = "media/cards/piercing_bite.png"; // placeholder

            pbite.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                int oldBlock = target->stats.defense;
                target->stats.defense = 0;

                std::cout << "[CARD] Piercing Bite: " << target->name
                    << " loses all block (was " << oldBlock << ").\n";

                int baseDamage = 3;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Piercing Bite: " << target->name
                    << " takes " << totalDamage << " damage.\n";

                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(pbite);
        }


        // --- SPIKE TRAP ---
        // Basic 1 Trap Enemy:
        // When an enemy ends their turn without playing a card,
        // deal 5 damage to that enemy.
        {
            CardDefinition trap;
            trap.id = nextId++;
            trap.name = "Spike Trap";
            trap.type = CardType::Trap;
            trap.school = School::Generic;
            trap.manaCost = 1;
            trap.targetScope = TargetScope::Self; // you arm it on yourself/your side
            trap.texturePath = "media/cards/spike_trap.png"; // placeholder
            trap.trapKind = TrapKind::SpikeTrap;

            // Behavior is handled in CardGameState::triggerOnEndOfTurnTraps
            trap.effect = nullptr;

            cards.push_back(trap);
        }

        // --- GEAR UP ---
        // Basic 1 Sorcery Self:
        // Gain 2 strength and 2 block.
        {
            CardDefinition gear;
            gear.id = nextId++;
            gear.name = "Gear Up";
            gear.type = CardType::Spell;
            gear.school = School::Generic;
            gear.manaCost = 1;
            gear.targetScope = TargetScope::Self;
            gear.texturePath = "media/cards/gear_up.png"; // placeholder

            gear.effect = [](const EffectContext& ctx) {
                ctx.source.stats.attack += 2;
                ctx.source.stats.defense += 2;

                std::cout << "[CARD] Gear Up: " << ctx.source.name
                    << " gains +2 strength and +2 block (now "
                    << ctx.source.stats.attack << " strength, "
                    << ctx.source.stats.defense << " block).\n";
                };

            cards.push_back(gear);
        }

        // --- RAMPANT GROWTH ---
        // Aura, cost 3: Gain 2 additional mana on each of your turns.
        {
            CardDefinition ramp;
            ramp.id = nextId++;
            ramp.name = "Rampant Growth";
            ramp.type = CardType::Aura;
            ramp.school = School::Generic;
            ramp.manaCost = 3;
            ramp.targetScope = TargetScope::Self;
            ramp.texturePath = "media/cards/rampant_growth.png"; // placeholder

            ramp.effect = [](const EffectContext& ctx) {
                ctx.state.applyAura(ctx.source, AuraKind::RampantGrowth);
                };

            cards.push_back(ramp);
        }

        // --- SWALLOW ---
        // Basic 2 Sorcery Enemy:
        // Deal 10 + attack damage to target enemy. If that enemy dies, heal 10 + healBonus life.
        {
            CardDefinition swallow;
            swallow.id = nextId++;
            swallow.name = "Swallow";
            swallow.type = CardType::Spell;
            swallow.school = School::Generic;
            swallow.manaCost = 2;
            swallow.targetScope = TargetScope::SingleEnemy;
            swallow.texturePath = "media/cards/swallow.png"; // placeholder

            swallow.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                // Damage
                int baseDamage = 10;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Swallow: " << target->name
                    << " takes " << totalDamage << " damage.\n";

                ctx.state.onDirectDamage(ctx.source, *target, dealt);

                // If target died, heal the caster
                if (target->isDead()) {
                    int baseHeal = 10;
                    int totalHeal = baseHeal + ctx.source.stats.healBonus;
                    if (totalHeal < 0) totalHeal = 0;

                    int oldHp = ctx.source.hp;
                    ctx.source.hp = std::min(ctx.source.hp + totalHeal, ctx.source.maxHp);
                    int healed = ctx.source.hp - oldHp;

                    std::cout << "[CARD] Swallow: " << ctx.source.name
                        << " consumes " << target->name
                        << " and heals " << healed << " (base " << baseHeal
                        << " + healBonus " << ctx.source.stats.healBonus
                        << ") to " << ctx.source.hp << "/"
                        << ctx.source.maxHp << ".\n";

                    ctx.state.onHeal(ctx.source, ctx.source, healed);
                }
                else {
                    std::cout << "[CARD] Swallow: " << target->name
                        << " survived; no healing.\n";
                }
                };

            cards.push_back(swallow);
        }

        // --- RESHUFFLE ---
        // Basic 2 Sorcery Self:
        // Shuffle your discard pile into your deck.
        {
            CardDefinition reshuffle;
            reshuffle.id = nextId++;
            reshuffle.name = "Reshuffle";
            reshuffle.type = CardType::Spell;
            reshuffle.school = School::Generic;
            reshuffle.manaCost = 2;
            reshuffle.targetScope = TargetScope::Self;
            reshuffle.texturePath = "media/cards/reshuffle.png"; // placeholder

            reshuffle.effect = [](const EffectContext& ctx) {
                auto& deck = ctx.source.deck.cards;
                auto& discard = ctx.source.discard.cards;

                if (discard.empty()) {
                    std::cout << "[CARD] Reshuffle: discard pile is empty; nothing to shuffle.\n";
                    return;
                }

                int moved = static_cast<int>(discard.size());
                for (auto& c : discard) {
                    deck.push_back(c);
                }
                discard.clear();

                std::shuffle(deck.begin(), deck.end(), ctx.state.rng);

                std::cout << "[CARD] Reshuffle: " << ctx.source.name
                    << " shuffles " << moved
                    << " card(s) from discard into their deck.\n";
                };

            cards.push_back(reshuffle);
        }

        // --- SUPERNOVA ---
        // Basic 3 Sorcery Enemy:
        // Deal 5 + attack damage to target enemy.
        // If that enemy has an aura, remove it and instead deal 15 + attack damage.
        {
            CardDefinition nova;
            nova.id = nextId++;
            nova.name = "Supernova";
            nova.type = CardType::Spell;
            nova.school = School::Generic;
            nova.manaCost = 3;
            nova.targetScope = TargetScope::SingleEnemy;
            nova.texturePath = "media/cards/supernova.png"; // placeholder

            nova.effect = [](const EffectContext& ctx) {
                if (ctx.targets.empty() || !ctx.targets[0]) return;
                CombatPlayer* target = ctx.targets[0];

                bool hadAura = (target->aura.kind != AuraKind::None);

                if (hadAura) {
                    const char* auraName = nullptr;
                    switch (target->aura.kind) {
                    case AuraKind::RainyPresence:  auraName = "Rainy Presence";  break;
                    case AuraKind::RampantGrowth:  auraName = "Rampant Growth";  break;
                    case AuraKind::FireSpitter:    auraName = "Fire Spitter";    break;
                    case AuraKind::Waterfall:      auraName = "Waterfall";       break;
                    case AuraKind::DeepRoots:      auraName = "Deep Roots";      break;
                    default:                       auraName = "Unknown Aura";    break;
                    }

                    std::cout << "[CARD] Supernova: removing aura '" << auraName
                        << "' from " << target->name << ".\n";

                    target->aura.kind = AuraKind::None;
                    target->aura.remainingTurns = 0;
                }
                else {
                    std::cout << "[CARD] Supernova: " << target->name
                        << " has no aura; dealing reduced damage.\n";
                }

                // Damage: 5 base if no aura, 15 base if aura was present
                int baseDamage = hadAura ? 15 : 5;
                int totalDamage = baseDamage + ctx.source.stats.attack;

                int beforeHp = target->hp;
                target->takeDamage(totalDamage);
                int dealt = beforeHp - target->hp;

                std::cout << "[CARD] Supernova hits " << target->name
                    << " for " << totalDamage
                    << " damage (" << (hadAura ? "full" : "reduced") << ").\n";

                ctx.state.onDirectDamage(ctx.source, *target, dealt);
                };

            cards.push_back(nova);
        }

    }

} // anonymous namespace

namespace LizardCards {

    const std::vector<CardDefinition>& getAllCards() {
        initCards();
        return cardStorage();
    }

    const CardDefinition* findByName(const std::string& name) {
        initCards();
        auto& cards = cardStorage();
        for (auto& c : cards) {
            if (c.name == name) return &c;
        }
        return nullptr;
    }

    void buildStartingDeck(School school,
        std::vector<CardInstance>& outDeck,
        int& nextInstanceId)
    {
        initCards();
        outDeck.clear();

        auto& cards = cardStorage();

        auto addCopies = [&](const std::string& cardName, int copies) {
            const CardDefinition* def = nullptr;
            for (auto& c : cards) {
                if (c.name == cardName) { def = &c; break; }
            }
            if (!def) return;

            for (int i = 0; i < copies; ++i) {
                CardInstance inst;
                inst.def = def;
                inst.instanceId = nextInstanceId++;
                outDeck.push_back(inst);
            }
            };

        switch (school) {
        case School::Fire:
            addCopies("Fireball", 3);
            addCopies("Kindle", 4);
            addCopies("Detonate", 4);
            addCopies("Fire Spitter", 2);
            addCopies("Sparks", 4);
            addCopies("Magma Armor", 3);
            addCopies("Explosive Trap", 2);
            addCopies("Fuel", 2);
            addCopies("Wheel of Flames", 2);
            addCopies("Volcanic Eruption", 2);
            addCopies("Meteor Storm", 2);
            addCopies("Dragon's Breath", 1);
            break;

        case School::Water:
            addCopies("Rainy Presence", 3);
            addCopies("Hydrate", 4);
            addCopies("Dive", 4);
            addCopies("Clear Water", 3);
            addCopies("Ice Shivs", 3);
            addCopies("Splash", 2);
            addCopies("Ice Block", 2);
            addCopies("Wash Away", 2);
            addCopies("Brainstorm", 3);
            addCopies("Tsunami", 2);
            addCopies("Waterfall", 2);
            addCopies("Frostwalking", 2);
            break;

        case School::Plant:
            addCopies("Green Lotus", 3);
            addCopies("Vine Whip", 4);
            addCopies("Snake Trap", 2);
            addCopies("Snuff Out", 2);
            addCopies("Blossom", 2);
            addCopies("Aloe Vera", 2);
            addCopies("Vigor", 2);
            addCopies("Deep Roots", 2);
            addCopies("Regrow", 2);
            addCopies("Vampirism", 1);
            addCopies("Bountiful Harvest", 1);
            addCopies("Nature's Wrath", 1);
            break;

        case School::Generic:
        default:
            addCopies("Rampant Growth", 5);
            addCopies("Scratch", 6);
            addCopies("Piercing Bite", 6);
            addCopies("Spike Trap", 10);
            addCopies("Gear Up", 4);
            addCopies("Swallow", 2);
            addCopies("Reshuffle", 1);
            addCopies("Supernova", 2);
            break;
        }

    }

} // namespace LizardCards
