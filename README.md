## ░█░░░▀█▀░▀▀█░█▀█░█▀▄░█▀▄░▀█░░▄▀▄░▀█░
## ░█░░░░█░░▄▀░░█▀█░█▀▄░█░█░░█░░█ █░░█░
## ░▀▀▀░▀▀▀░▀▀▀░▀░▀░▀░▀░▀▀░░▀▀▀░░▀░░▀▀▀ 
## (Digital Edition)
Lizard101 digital edition game created by:

Benjamin Rimon
Judy Lin
John Cheek

Based on the NCSU VGDC Spring Game Jam 2023 card game by:

Benjamin Rimon
Caleb Young
Gloria Chien

-----------------------------------------------------------

Instructions to run:

* Open the build/ folder
* Run Lizard101.exe

-----------------------------------------------------------

Gameplay and Controls:

Survive and defeat three increasingly difficult rounds of enemies.
After each victory, select new cards to upgrade and customize your deck.

* Controls

Select card	           Left-click
Drag card to target	   Hold left-click and drag
End turn	           Press E
Exit game	           Press ESC (Or close the window)

* Cards & Types

Cards belong to one of three elemental schools:

Fire, Water, and Plant

Cards cost mana to play.

Players gain 2 mana at the start of each of their turns.

Players begin with 20 life.

* Card Categories

Sorceries

Trigger immediately.
Apply their effect once.

Auras

Continuous effects lasting 4 rounds.
Only one aura per character, new auras replace the old one.

Traps

Played face-down.
Trigger automatically when their condition is fulfilled.
Can be used to block or punish enemy actions.

* Targeting Cards

Cards that affect one specific ally or enemy must be dragged directly onto that target.
Cards that affect multiple targets (all enemies / all allies) can be dragged outside 
of your hand area and release to cast.

* Progression

After each round:

Enemies become stronger
You can choose new cards to add to your deck
Defeat all three rounds to win the game!





*** Known Bugs/Changes to be made/Crash Log ***
Basic type cards not in pool for between-round card selection, but ephemeral cards like shivs/drain are (when they shouldn't be)
When hand is very full, cards overlapping are not prioritized in the way that the player expects.
Reduce enemy hitboxes to prevent forward facing player misclicks
Order of effects is not really correct, ex: Rainy Presence applying buff before card is played.

Starter decks feature no basic cards / all cards of type
Enemy decks all the same, should be different and have typal cards
Auras should be updated with a texture rather than just colored rectangle
[Allow window to be resized, zooming camera to match, update textures to be higher resolution]
Ability to re-order hand for discard effects?
Firespitter costs 0 mana, but card displays 1 mana
Drain costs 1 mana, but card displays 0 mana
Fuel card misspelled Feul
Cards shouldn't be able to target defeated characters
Defeated characters should lose all stats and auras
Certain card wording (Fuel, Waterfall?) should be updated for clarity

Game sometimes crashes when third level is loaded, no background is loaded
Restarting PC after game is run sometimes causes blue screen microsoft fixing things

Last Updated: 12/10/2025
