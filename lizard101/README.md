# **Team 17 Game Engine:** Judy Lin, John Cheek, Benjamin Rimon  
**Installation and Compilation Instructions:**   
**Option 1 (The easier one):**

1. Download the engine ZIP and extract  
   * [https://github.ncsu.edu/jwcheek2/CSC481-game-engine-team17](https://github.ncsu.edu/jwcheek2/CSC481-game-engine-team17)  
2. Open the extracted engine folder in Visual Studio 2022 with the Desktop Development in C++ package installed   
3. The green compile button at the top should default to the target: ‘Main.exe.’ If it doesn't, try Option 2\.  
4. Click ![][image1] to run the engine  
5. Done\! 

**Option 2 (The annoying one):**

1. Download the engine ZIP and extract or clone the repository  
   * [https://github.ncsu.edu/jwcheek2/CSC481-game-engine-team17](https://github.ncsu.edu/jwcheek2/CSC481-game-engine-team17)  
2. Navigate to the engine directory in MSYS2 or MINGW64 terminal  
3. If present, delete ‘build’ folder: `“rm -r build”`  
4. In the terminal, execute: `“mkdir build && cd build”`  
5. Execute: `“cmake ..”`  
6. Execute: `“cmake --build .”`  
7. Execute: `“./main”` or `“./main.exe”` to run executable  
8. Done\!

# **Task 1: Core Graphics Setup Documentation** **Relevant Files: ‘main.cpp’, main.h’**

**Beginning of main method:**  
![][image2]  
`SDL_Init(SDL_INIT_VIDEO());` Initializes the SDL library.   
If there is an error, log it to the console and return an exit status of 1\.

`SDL_CreateWindowAndRenderer(“Team 17 Game Engine”, WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer));` Creates the engine window and renderer with the name “Team 17 Game Engine” and the default size of 1920x1080. (constants declared in ‘main.h’) Initializes the window and renderer to their respective pointer variables.  
If there is an error, log it to the console, quit SDL and return an exit status of 1

**At the top of main game loop:**

# 

`SDL_SetRenderDrawColor(Renderer, 0, 0, 255, 255);` Prepare the scene and render a blue rectangle as the background.

# 

# **Task 2: Entity System Documentation** **Relevant Files: ‘entity.cpp’, ‘entity.h’, ‘entityManager.cpp’, ‘entityManager.h’**

This task implements a generic entity system for the game engine. Entities are managed via the Entity class and EntityManager singleton. The engine can update the positions of entities, handle collisions, and render them to the game window/

Entity data is primarily handled in entity.cpp and entityManager.cpp, with class definitions and prototypes in their respective header files.

**2.1: Understanding the class system for entities**

**Entity.h:**

The Entity struct contains the following fields:

* **std::string name**: String identifier for the entity.

* **float x, y**: The entity’s position in the engine window.

* **int width, height**: Dimensions of the entity’s sprite.

* ***SDL\_Texture texture*****\***: Pointer to the entity’s sprite texture.

* **bool physicsEnabled**: Enables physics on the entity. Defaults to false.

* **float velY**: Vertical velocity for the physics system. Defaults to 0.0f.

* **bool isSolid**: Determines if the entity blocks other entities. Defaults to false.

* **bool useSpriteSheet**: Enables rendering from a portion of a sprite sheet. Defaults to false.

* **SDL\_FRect spriteSrcRect**: Defines a portion of the sprite sheet to render (used if useSpriteSheet is true).




**Entity.h contains the following prototype functions:**

- \****Entity(std::string name, float posX, float posY, int w, int h, SDL\_Texture*** **tex \= nullptr, bool physicsEnabled \= false, bool isSolid \= false)\*\*:** Constructor to initialize an entity.

- **virtual void update(float deltaTime)**: Updates entity state, including physics and collision handling.

- **bool collisionDetection(Entity& a, Entity& b)**: Returns true if two entities intersect. Logs collisions to the terminal.

- **\**void renderEntity(Entity& e, SDL\_Renderer* renderer)\*\*:** Draws the entity to the window, using the entity’s texture pointer or sprite sheet

- **SDL\_FRect getBounds(const Entity& e)**: Returns the entity’s bounding rectangle, used for collision detection.

**EntityManager.h:**

The EntityManager class manages all entities within the game engine. It is implemented as a singleton so that the entities can easily be accessed by other systems in the engine. 

**EntityManager.h contains the following field:**

- **std::vector\<Entity\*\> entities:** A vector array of all entities contained within the entity manager


**EntityManager.h contains the following prototype functions:**

- **static EntityManager& getInstance()**: Returns the singleton manager instance.

- **\**void addEntity(Entity* entity)\*\*:** Adds a new entity to the manager.

- **void updateAll(float deltaTime)**: Calls update() on all entities.

- **\**void renderAll(SDL\_Renderer* renderer)\*\*:** Calls renderEntity() on all entities.

- **\**void removeEntity(Entity* entity)\*\*:** Deletes and removes an entity from the manager.

- **\**Entity* findEntityByName(const std::string& name)\*\*:** Returns a pointer to an entity by their name, or nullptr if not found.

**2.2: Using the entity system to create new entities and render them to the engine window**

**To instantiate a new entity, first, create an EntityManager:**   
`‘EntityManager manager;’` 

**Then, instantiate a new entity:**  
`Entity* myEntity = new Entity("Player", (float)startPosX,(float)startPosY, width, height, texture, true, true);`

**Finally, add the entity to the entity manager:**  
`‘manager.addEntity(new Entity(myEntity));’`

The main game loop will repeatedly call `‘manager.renderAll’` and ‘`manager.updateAll’` to render the entity to the screen with the initial position and texture specified in entity construction and constantly update it. 

**2.3: Entity subclasses**

Additionally, the engine contains three entity subtypes: Player, movingPlatform, and stationaryPlatform. These classes are implemented as subclasses of the generic entity type and have additional functionality. ‘Player’ is controllable with WASD input. ‘StationaryPlatform’ is a collidable platform placed within the game window. ‘MovingPlatform’ is a platform that can be set to move vertically or horizontally between two points.

# 

# **Task 3: Physics System Documentation** **Relevant Files: ‘physics.cpp’, ‘physics.h’, ‘entity.h’**

**3.1: Understanding the physics system**

The physics.cpp and .h file manages the physics system for the engine. Currently, the only physics feature built into the engine is gravity, a constant downward acceleration applied to any entity with gravity enabled. This can be demonstrated when the player character jumps up and then falls back down according to the physics system. To have an entity utilize the physics system, set its ‘physicsEnabled’ field to true upon entity creation, it is false by default.

**3.2: Physics system implementation** 

‘physics.h’ contains the private float `‘gravity`’ which determines the downward acceleration applied to entities with gravity enabled. Each entity struct has a ‘`velY`’ variable to represent its vertical velocity, this value updates according to the gravity strength. `‘gravity`’  is initialized to 980.0 by default, but can be set by simply doing: ‘`physics.setGravity(newGravity)’` 

**Note:** This gravity value applies to all entities globally and cannot be set on a per-entity basis. 

To ensure the game is using the engines physics system, add: `‘physics.updatePhysics()’`   
To the main game loop, and pass your the engine’s entityManager and deltaTime value as parameters. 

The updatePhysics and applyGravity functions look like: 

While the collision system prevents entities from colliding with other collision enabled objects, this collision logic within physics.cpp’ applies gravity and prevents the entity from falling out of the game window.

# **Task 4: Input Handling System Documentation** **Relevant Files: ‘Input.cpp’, ‘Input.h’**

Data for mouse and keyboard input is managed by Input.cpp and Input.h

**Input.h contains the following prototype functions:**

- `void update():` The update function is called every frame to detect keyboard and mouse button input. It also achieves multi-button input using the following `‘keyboardState = SDL_GetKeyboardState(nullptr);’`  
- `Bool isKeyPressed(SDL_Scancode key):` Checks if the given key is pressed. Used for getting keyboard input.  
- `getMousePosition(float& outX, float& outY):` Returns the mouse position, giving both x and y coordinates.  
- `bool isMouseButtonPressed(Uint32 sdlButtonMask):` Checks whether a given mouse button is pressed.

**4.1: Understanding the class system for entities**

**Input.h contains the following public fields:** 

- `‘static float mouseX();’` The x position of the mouse in the engine window  
- `‘static float mouseY();’` The y position of the mouse in the engine window

# 

# **Task 5: Collision Detection System Documentation** **Relevant Files: ‘entity.cpp’, ‘entity.h’**

Data for entities managed by entity.cpp and entity.h

**5.1: Understanding the class system for entities**

**Entity.h contains the following function for collision detection:** 

- `bool collisionDetection(Entity& a, Entity& b):` The function uses the SDL bounding box collision  (SDL\_HasRectIntersection). It takes in two entities and returns a boolean value to determine whether the entities are overlapping.   
- `SDL_Rect getRect(const Entity& e):` Helper method that converts entity into SDL\_Rect for collision detection


**5.2: Collision Detection Integration into Game Engine**

The `Entity::update(float deltaTime)` function calls collision detection when updating positions. If a collision is detected and one or both entities are solid ( `isSolid = true` ), the engine prevents them from overlapping by adjusting their positions or stopping their velocity. If the entities are not solid, they will be able to pass through each other. 


# **MS 2 - Task 1: Timeline**

**Relevant Files: ‘Timeline.cpp’, ‘Timeline.h’**

Timeline is a time management system, which works simply by keeping track of game time based on frame rate. Timeline has the ability to pause and unpause, and change game time to 0.5x speed, 1.0x speed, and 2.0x speed, which is currently built directly into the timeline class to use the 1, 2, and 3 keys respectively. Any function can theoretically call setScale() to do this as well.

Timeline.h contains the following public prototype functions:

- void update(); - The update function is effectively the main time loop of the game.
- void pause(); - pauses the game, including all moving objects.
- void unpause(); - unpauses the game, including all moving objects.
- void togglePause(); - A flip-flop for pausing and unpausing the game, depending on which state the game is currently in.
- void setScale(float scale); - sets the speed at which time moves in the game, this affects all moving objects.
- float getElapsedTime() const; - gets the total elapsed time the game has been running.
- bool isPaused() const; - true if the game is currently paused, false otherwise.




[image1]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAFMAAAAXCAYAAAB3e3N6AAABnklEQVR4Xu2VMXLCMBBFuYkrqlT0dKKmo0gtSjo6F2TGF8gNdIOcQLlBetW5ieKVvPZKcrCENUwys8UbzPp75TytyKZpGsvUYRMXmMdhmRVhmRVhmRVZlHn7vtnj2zGpMylZMpHdYZfcZyaKZAKXz0uSYTzFMp85pcbopPaXeVjms4T+J1bJBE7vp+QZijHKNqLrP401Sg414whzvoaZ8Vl3LYdr+DRWyXQdQHQ67C0V6eHXiLO/9ULatp3erQe+xxlktcztyzZ5hkJf2F0rf3SlgpfDP1SMed1nOoF5KlMPooXrI6N1Ot33053/3m8erqnGfmJ8hmZhvSWhOSKBh2XuX/dJdg46GW5SgsmbJoXuvu683HQyfRY2IhTgBYdMeRBKN+5edg3FMq9f1yRzjxyZMClYg+NXLjPcmBiUlpNdQ5HM3GmklMkUVusymdjDTR8ec+g5/FRA3R3v/ujjMQ+zYsyuJVtmXM8lRyZew73SyaQ93G8hmUL3T4YIns2S+2tZlHn+OCc1Zp5FmUw+LLMiLLMiLLMiLLMiLLMiP9c6XAXWRobBAAAAAElFTkSuQmCC>

[image2]: <data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAgEAAACjCAYAAAATrc2tAAAzVUlEQVR4Xu2d+VsVSZrv5y85LLKouKIoKu6igAqlIpTlUu77Vu6Uu4iKCyigsqqIG2IpBUet0tKy1VasKsutQFrp5bkzc2ee+/Qs3XOn585MT703IyIjM3JBEM+xDp7vD58nIt54Y8mEc95vRmbG+RuPx0OS6IW3aeQUs+xPbl8nGuJiN+lGddVFFlvEtDoaO2u4i2/nIKO8yWELNLzeMoet2uu1lOtsZQAAAJ2Tv7Eb3sjoEzRx+Uyn3Q9c6EigGZRHaevXO+0BQodFwII8yk6PcNr9QHtEAOPS4cUOGwAAgM6FIQKi4sdTdPx6itHL/bc0Udrhn2jE3h8o43AVt0VPOkcT16zX/MY7OhLEUunKNH6Vf+4aUSKzRS2nTdufcpv0Gz1ypqVcq+Ur9nxDtV7T5rUFnsiBbH5LqU93Ue615jttfs9o2M4Gyii6wm3RKSX0UdaeN8zPQzNmzKCNm7OpZNc8Xh4YH0+l2xbTJ0mDqLS0lNtKC3dTfGIG5ZWI8qhFObR/x2e0cvNu2rN4hKNPV8Km8KAft6DSCP4sHZw+2xQD/RZT3KxcSitupMgQ0W5kYRMNP/iCBmc/oaT5ybT3Yj1VHC2nwryT5D2T7RxHI2rQRFq4aoMx/y49+tPWwlLav2k+rd53jNtKtLoFU8dq/eRR7pIRlLRyP61KjuJ1sh2jvSLAzQ8AAEDnwrYSMNMiAvrreSNotbkSEEvbx4ql/sPVRMmaLXHlK15Wgz7DLgJ4PtbL27C8XQQIJlpEQHx/kTfm146VABbwJIbt0CrTJ32jkd+riwDVt70MO9JkzFUi5znueBNFammvZbe4jZE0f4zRLloXBBKv94omkNzOh97v2n2OY8oqsM65tDSXp30/zuIiQNhKeXnDlJ7KWM7g7ioCLh122AAAAHQu3k4EDDxGGQcrHZ2YOEWADPZzc/9k8W1LBBR94Qw8bYqAPtmUcaTGpZ1kOCWHi7xFBBxYofgk0bwBoRafzcdKKcrR15sJn1xDGcdvWWx2EWDMO2yKRQSwOrVdiEv/KnKeS/aIq36GUwSI8rJ9xw0RsOFwKRXYBE57RcDRVQMcNgAAAJ0LQwTIK1JBvbsI0Bi4Xly9xrp05iYCIobl0e1rf6U4rU4EfpGqqwNuIoA9GFh71rzatM7vlrsI0Oi/+jovD4i1z01QoAX0woO7xZXz1hnKysABw2fP4SI6eijHWAngtvwC7jck1Ozry+J1jv5VopL28bl8VNDAy3YR0GXUXm4bmjmJp+Ge5cYx2vt6E31S5lFp8VEK9Yzgc2QCwL4yENpzDL8l0F+78t+7aJiwRyVR6XHrLYb2iIDaeqcoAAAA0Pl4uwcDg4QZMxbQ9j0s6Jc46kwiaEUfuy1wYc9CrN240SIMAAAABDcQAQAAAECQAhEAAAAABCkOEZCfn+9wAgAAAMCHh0UEFLg8BQ4AAACADxOLCJhz4LLDAQAAAAAfJoYI2FHlpUvVJx0OAAAAAPgwsawETNp5xuEAAAAAgA8TiwjYVolnAgAAAIBgwSICpu4+T+tXKfvoAwAAAOCDxfGKIAAAAACCA4gAAAAAIEiBCAAAAACCFIsI+OyzzxwOHWXo0RTq1cVqyzyV4PADAAAAwC+D30RA5rlkq61vb4pw8WOknUvR/AXdXOol4R8PotEzw5QxUhw+FnrEGP0mLo022jDGb+zBywMPtNEHAAAA8IHiJxEQSgkpIRZbehsBO6mNejfaEgFufSYvj+LpJK0uygMRAAAAIHjxyzMBLMCq5ZCR/eijrO4OPxVLwO7anTLPjuN5a6APpz5Km7ZEgMcTIq78q8YaNikCwiYN5HmIAAAAAMGK71cCQsIp8/RQiy2jzWDtFAFT82J5/t1EgE5IKGWUDOR5KQK6LBxKYz4NgwgAAAAQtPhcBLgF/HGLIhw2lX7ze9NErV2cloYzm4sIiB4SqRFNcSwd3IXbEitTaMSavjR0+2BHn7LtqI2xNGTLUJqyrw+3pe7qR4M2DDL6ZSKAjSvo5egDAAAA+FDx+e0AecUt6bs90eEDAAAAgF8en68EAAAAAKBzABEAAAAABCk+vx0AAAAAgM5BwIiAkOiR5PV2rp8yLqmu7XRz9jlDV2rnoM5iY+dkbVrbD1leqffi/AEAwC+IX24HvDz1e0u5ZP9vqbH4EZWvO+vwlbgFg7qTW4z8xppSamm5yJnVL5Rqnom8LDOflpYCRx92ZBuGvc7uN1gt37P+xHLooFW0ZaL51sOUrf9It68TZ2b/SMquFHlZZj63r//JMY4d2YbByqyfzaNF3bXjl1x91PLt2j84+vR4Ei125he3sLnVfm7V/QsvH7uk9Kv7XL+m+9T+g9J/pOPvV1nrpem97POw4j2902EDAADw/vC5CGjWBADDbrP7WQjpRYsjnPbavAVG3h60mQiI0/Ov9br2iAB7X6trTtD2c4c023lLveqjCodxus0e9GSQlLDgPVzP3zKCbNsiwN6XFBMsL0WA8PlPSxt1PDdkH2HjvqXqbasNe8XqDMVHn1/8ebp5+g4XAdZ+ulDBrP48P+/gv9HuCWbdgjynqPJ6Wxd9ot4p/AAAALw//HI74EKhGfSlKGDcXpTp8OV0tV5lS1QREDkigwfhwuX9eFkVAd++FPmOioBRYSzfi2qzehr2lzbRYV8JsAewsN5ZPNDumj2dl9WgzIIpy3dUBAxe/luaE/NuImDZ4X+nuTFiLv0Vu6sI0PPqSoDoew5lyLZjvqUvsjcZ/kNXFjnGtJ8je928pLZvGQAAAPAfPl8JYKgigNHRlQA3Eg/up3sl8T5dCZB51q/Mv60IkPT59AVdydliCco33nElQNh+ficR4PH0pNtfsFsAf7XYXUVA78N0u+aFy0pATzowVdza+DT3z5T7kVm3vMCcmwQrAQAAENj4XARMHJxGl4t+z9MY3damCNDwXrFfSYaQt/6MUWbL9aVHFvGAnz9WPBNQlLeIGl9fpFc/bNN9yihPszH2rBzkGIPB6pgIYCkru4mA1NShXASw1Bz/Ih9/62BRnr7vgqXf29f/h3av+4JuagF890TxTMD2z76gq1eJbl1q0X3+Qhs0G2PNp+YqhwqrYyKApawsRUD6rj8aIkD4/LfhI/2Gu/Snwvr1Hqng+egR+3n76kM3jH7E/K5yP/a3YyJAztf0Idqmz1Ht2x7QM7NPU+n6UY45WNpcOOCwAQAAeH/45XZARwjpNt4RSN4V9V6+/ZkCX1B1ue6d5iyX2iX2+o7Clv790a8r/O2AegpVbOycbJlurqq0Rr0XbwcAAMAvScCIAAAAAAC8X3x+OwAAAAAAnQOIAAAAACBI8cvtgA9BTKw+cBL3qwHQOX25nso2iJ/sZp+Lg6vHm/Uf7yFvvfPtEABA4OOXlYB37yeSP8hXVJVtPNDX0nKO8vKW8jJ7z32H9xQtcrSz8qCDDwMeveylObFW21cLu9PfPbpplJsqcqhiXhp9V5rjaP8mmiqyHTZuL1vrsEkWrt5ElzPM8rWK9o15r2Cjw2bnu1zxlgQnfBj9lOvcy+FhwRqHTYWdC7utaPs6h+2tCOnN+y1Zu9Lon5273Fkf8XKUVj6d7xy3LaJT51BUSBQ1lW9w1Kl8z/7WMTPpH3912FHXGuzNlVvVn9FvtLR8uoe+0d9CufP9WXrm/ZT7tDS3PWf2P37ryga6dKeU5kQ56xk98w862sg3Y+SbL77CTQz3mb2PapR9PFrzAwAENn4RAe9Mwkx61bDeYmtpqRL57inU8jynwyJg9Opl/AtzqLIvARccS2Lp5iHxWqHbl9m1uV3pD7oISF+6lq7PjrH5hNDj4myqXTSal/fvyKEt3B5mCfBWERBCTzQRcWfLVMOHBeFHWj9HJ5r995i1jGrSRZ4JAhYEJdY5mNjrWX7jsqX0JF/udxCm+5jzUfv9WCtHJc107een4+Z2ztJmL6u2a7Z+mS1m0Dg+dmYrQY6N/cP6RFu/+jbDXUZRY9HsVkXAN0XuQovRPXOh3tdWGjtlJk3U7Q8Lt/D5rdfLX91kf+t+9He11mN9E/Y3UJgIsNe1KQLC4qk41Wl/3HieGp+X8vyFH5xvvdjHzv9G1B25fpxeN1caPneK4nl64JOu3FZaV6iVzdddWR07Bywdq/9tvN5yx3yE3fo52VFVR1O6meX6K6cdbQAAgUXA3g4I7zWEfxFdK0rhZUME6PmOigApLuSXZrPhE/NGEaDCgo8MZhIZ9MYsWE3fLoprlwiwBFfdp6lspaPODguqdpsb9uDNUiYo7iwZqNiVgNnKSoDbXNz6VnHOMdT0ixhDz7ZO4Pkvi+1+Jt16D+NtSnVBZIgAPd+aCGiLmMETePvx8bG8vP1zLd+KGHlbHry4QC2vT/G8KgJ+aKcIiFq6mafsSp/9j07X8rXPzX4aTiUZ9Wo7uyhgyDGjV26hBycSyROVasyN+RXfvWgIDtmO9fvqLvsfDCG2+sZs3jL3VRP75+TjPdW0Qd9HAwDQOQhYESAxr3R0EaAFqpbmIx0WAU+viCD33OjX9GmvCGBX498sEAFEIgMcu4J9npXUcRFQssBRZ8cZYN1xC9TvKgKayrZT7qw0175V7HO0+ESJ9hJ7WzvSxxAB2tVyU+nSjomAsEFU80l/uqP3eWNeb54mJyfR0/Ic+ryHS5u3ZGbVcfLusIoA4/+4DRHAVjnMlYAuXASwfmSAb/bO4nVuIsDelxQBBpoIePXA/F+8+ONFWmZrz/plc1fbtXclYPf5estKAAAg8PHL7YB37SetlC1Rii+9189yuc240nl9lpeZCDBs9i87HSYC7D5sh0GWb7wn7ln3nPeZGOfFfkMETM0+Q+cOzHP0p3Ipf5cliCXPWmINaqF9ef7Z9lSeCkHgoSe6T8kIa5u3EQHhA80gaq/jaMHcHmRlKkUAS+0+0o9xPs26jK/2Y6DP9ZOlay1l1YetmKj9sONmPvVF2Y6xVfZuM+uf7hHCxOx3By8zEaCOpR6DvT87S1dv4H4zwz10aJfZB3vWwO4rGbPRPRhK7P9r3yjlKfpKg+rDr85d+nn8G9OHiQBxVa6WGWGWsdR+GUwAqPWqz4npsg+z3+Nzulh81PlU13vp4z7ilzole87UUXZGd8XWh28apfp4vbWWMgAg8PCLCPilUL8EH5+f6Kh/I1rgPJRslrcVX3Bc6YDgZmP5lw7bu/KNLXjb6wOFi19a3w4oylJWjNjbAd46RxvvyW0OGwAgsAj42wH+ZlLFUXr+7CRV7X9L0QCCjrLqaocNtE7WuAiHDQAQWPhlJcBX/QAAAADAf/hFBAAAAAAg8An62wEAAABAsNKpRcDjU7932HzB1R/Pa5gbqAAAAAAfIn65HfCu/Wzd/BM1awFewmyzl963lNX65lPNwlZy26gzfV5Y2j3LPi3KFd87xlWxi4A6r5d2TIh0+AEAAACdFb+IAF9wVw/a4cPFJkHNp34nykNK6Ou5qTxvXwmwiwA1f08Kg2PXHT5u2EUAAAAA8KERsLcDpAiQuAVthwgovePwtbdrLhDvYtvtdiACAAAAfOj4ZSXgXfthAkBdwmfEJevL+BpJum3UtGsWv5d6nokDtiog6yyioB0igAkAibSdq/dSbxdfAAAAoLPiFxHwIYLdAwEAAHxoBOztgECjGrvFAQAA+MCACAAAAACCFNwOAAAAAIIUn4uA/MNHHT8pCgAAAIDAwy+3A/AQHQAAABD4+HwlgAERAAAAAAQ+fhMBvbo47QAAAAAIHHA7AAAAAAhSIAIAAACAIMXntwPYpjrT4rs47AAAAAAILHwuAgAAAADQOfDL7QAAAAAABD5YCQAAAACCFIgAAAAAIEjB7QAAAAAgSMFKAAAAABCkQAQAAAAAQQpEAAAAABCk4JkAAAAAIEjBSgAAAAAQpEAEAAAAAEEKbgcAAAAAQYrfVgLCE3tQz/5WW9z83g4/AAAAAPwy+E0EZJ5LsdlCachYpx8jTfNl/oxuLvWS8DlDaOyCN/9CYUJhCvVysQvCxTgn4y320HFxmj3Zxd8/DC9JoT4u9kDF+bd0Z2perMMWSAT6/AAA4H3jJxEQSgkpIRZbehuBJKmNejfaG5yshDtEwPvmfYiAXp+Pcdj8zbsG2bBJAx02X/Ku8wMAgA8NvzwTMPSoLTiHaIH3xGCHn4pFBHTtThknx9CA5QOMQB85JJKih0QbwTNaK7M6lkYPiTBs3WYNMvoJHdWD4ub3oSlVKRRijGUTASGhWrsoyjw93LCJPkX/rNxj3UhK3BhLE060vlrQc1Zvbb5sRUG06bJ4GH1UmkhDdo6kzKqR3Mbqeo2KpCmVrYsA5jNyTV/h6xHHnXlyMCXM6Gr0zVJ2XLIcEteNBm0YZDlX/XaMMY6D2RIKk2lKUQJNPpNC8SOc4zKSl0dR+lntGMMjRcAMDbOcBz722XE0bEVvwxYxZwiNWhZDg3PGGEHWPj+WTtDo18PzRgHWlgiIHtePUtb3p67JvbR5jCVPH5YmUdyi/pRRLNqyFR15/li5tfkNnG8eAzvGKbv6UWwyO9YkwyduQazFJ7NqBHWznQ8AAOjM+GUlwL60nqgFvXAXPxW7CFC/sE2/cEvwdPsytgaSEBp7eDRNKhlBIzPMPpyBKNQiAhgZWt8RLuOY/diI7EITyxIptVLc0mAiQPrK9lMP9uXpm1YCpmyNcdgyz41TyqHUW8+zMaR9kjZ2ct5oo2xfCTCPIdTl+AXp+3qL896vL42aZq7kWESAfp6kLUWpk3+zMZ+G8VTOgfmyMZnISFkZ5RiXwZ4XGbh+KE9bfXakdx+aeti8mh9RqsxLn4c8V+wWE0vd5jci3ezPaH/SFI+ekC7G30o9j1IgAADAh4LPRcCYUykUZbNllLz5Co/RERHgdotBFQFG2x4xSvAOFVeRlnZWEWAXFyPL1ZUEN0L5VSLLTznXughgV9EsZfNuTQTYx+Y2m0DJrLSV9TYDc02xELNupGXOkzSf/gnWWzR22DiDRntosBb4urr0L31U2+DDel1ImPk3qxxmjMlS9rdloiKzMsEQMG60tRLAgrYqkrquGE7p+f0tPnYR4Dq/s85gbr9V4Pp3sIgxAADo/Pj8doAMdJLQlLg2AqjLg4EuIkDWc4ygGMLLU08kOnxYsIia0o/n49PEA4FyvIE7R2h9jOWrEwMPKP3qX/JqP7JNSvFYS9lO3JZhvJ4Ff5naRUC3GeJ2Qe8tY1oVAWxFYXJlMk09ZT8mM3B1GdWDpjJblZhv+LjePLCF6A8+Sj/WTyZb3tfLw3aP4PWDxrmMq48VqaeszFYs7OfCLgIYbC6jl3Uz/mY95ovbOPGTxUOc7ByzWxtjKls/f+1BzkM9d90+Fkv2HxWK1Q27CGDY5xfSM4rfIpLnxjhGVWyFhPLVIPZ/ovq4rdQAAEBnxecrAQAAAADoHEAEAAAAAEEKRAAAAAAQpPj8mQAAAAAAdA58vhKQ+qhW0HCRl0fKsobw6Uo9XdrZ+zDKMZkUrefDVu6lhPU9Hf522JiyjT8pb/nZYXOjvX7+JDp+PGWUNxnlKK3cY+41h58dtQ0AAIAPC5+LgAm16yxlFpBFPoQm1Kyi9oiAZL1NQvFiCp2z01HfFm8rAmLXFtPGRKfdVwSCCGDYA3rktC8dPnbsbQAAAHw4+Px2ALuKH12x0iibIoDVfUHtEQEJd0SbCY+qKepgCc9HjBtOkePGG21jSk5T0u2LFH/xNKX+6oAxdjfNb7wuAmKOV1LqvRM0oLKchm5ir4f14faJDy/zfvuneqjgipfKjp2kIwVnqLZso2MuDBbEj7f8lQZHaPlv67ktPnW6JbizfFFDC5W8/pkm9/VQyoV/pbKnv6N1D/+f4VempVvOf83LIXobz8rvqPxFU6tCYcWqVbRKwV7PGF3URClZJyip8CcKY7aRFTRh2TRepwZxe0C3iwBWP2rRVppS1kRxA8zVA5ZGDxzGfYYdaqKhOc8pftv3NH7ZTG5bWfQF1VfucMwLAABAYOPzlQCJXNK3igB2i6BtEdBdC949tYA98FotDbqh3BrQbKoIYEFc9MvEhYdG7RO7u8mVAPW2QqomKFjaP1UbP2QADbpp1nnrL1Cd1+uYh4QF6PxLubSraBQtmmTueGcXASxNKPnf3K/Epa685U88/ejKf9Caz2K5z4J7/0UbG3/W6v7iGJdRq83Lq2CvZzARIPKfUL9YzzuJANlPxoFy1zZDDjRRTKhzDgAAADof700EhM/eRMN3JFB7RAC7BTAkeyO/WldFRFsiIPVBhV4WImD4/VrqzzesCaWJ3k28btSRLJ6Os/TLaP3XCY9qwXreqDAqf/TAMvc3iYDNTT/T1MFh5IlcoYgAkR59/TNfVchp1sTF63+jxNN/pPIfHjvGbS8OERB/iNJzT5InNN4SxCfZl/Y1saDu7ih9e664S8mLMg1biOLDREB3tQ8AAACdFr+JgLfBeJhQI6VihqP+bQnr1/bDg++TEBfb+6JLuNPWGvar/vbCbgesHeq0AwAACGx8/kwA6Lx0VAQUfeF1/F4EAACAwCcgVgIAAAAA8P6BCAAAAACCFNwOAAAAAIIUn68E+GLHwNAF2WY/D8846tvibTcLCgTKG3/nsAUiYu+AbyzlKe14liDjsPh/AAAAEDj4XAT4YsdAJgKGLBd59pqfvb4t/CEC9l/yUjcXu6/oLCKAoYoABkQAAAB0TnwuAobeumzZpKcjOwYyETDy6DLqu/Nzoy+Z9iippPi5Yp+AlDMLyRPSh+RGQKkPKw1fuU9Ar4Giz4k3xY52bJOgxJMLKSR5I38dsdvRk/orfF1o+PaBRvsB09m+AT34XgRFJ6qNzXrWpfd3zFcwh8pf/pnn2X4Ag4//Pe0/t5f3W97y34adpQde/0xxHrGDIN/hj/noIqD8tdg06Li6B8ErYZPti26IfRFyXpo+x5/8k+7zH4btbZFvByQXN1Gkln6kBPe09esVvzeLgIyy+zztu/EH02YTAcMKmigiRMuPLKZw3dZlxCKLDwAAAP/it2cC7JsFCVv7dgyUKwF9zlZT/DyxiY99LwG3zYLcdgw0EUKBiQB1lUC9XSFXMVQRI7lSvKHVHfsEmgj40Qx6m56zXQBNmE2mrG6sUuZ1UgQobZhQEHV/sI5j65eRX71L8XGi7jro9TqPjyFFAAvQfbp3XASYviu4mOBtXESAWgYAAPD+8flKQHJdAfXdvsIiAlh5gh6YmQiI365d5evY2zPU2wGyn+F3tUB9v5KvDoR4Wtkx8JH47YCk+2KsyJ1HNNsVPs6ApZqYCImmhG9qKUbzMcaLyaDUB+e5D2sT0nsg7ydS9WmTaIpP3UvlT55Rz666rfsqHqRn5V2iLVdvcptdBOxp/pl2X/qSMqv+l0UE7P36R1pd95qX+W8UNP6tloptgKXPmpN1tKhaBFLmc/hyMcUnjXSZW/uxi4CEgz/R5P21NHLfE5qcfdriFzen1LiCH3O0idJ2nqXkLfm8nF7wiGKGjKd0RRywNkPm5NKAUT2oy4DxNLJQ/CaBOn7ixjJDNAAAAPA/PhcBHcF+lW+vf1sCbcfAYENdNXgbNpa5r1AAAADwD367HQCCl46KgJr6N91uAQAA4GsCYiUAAAAAAO8fiAAAAAAgSAkYERASPbKNp+9BMMD+B9b2ddoBACCY2XFavN2l2lg5Z/FQh6+di3Va2+K1DjvDL88EvDz1e0u5ZP9vqbH4EZWvO+vwldgPjlF3couR31hTSi0tFzmz+oVSzTORl2Xm09JS4OjDTvmtSqNdbnqko769FH7b+rEwWP8yn1aYR7VZ4mHFLbUnaddgp78dtX174MfUsMFht8Pmcq8knlbXnDDOw+vn+x1+bfHyLefXHupt/wP/+OgmrT5WQ3fWinJTRY7JsaWO9h0l49MFvM+rs/rw8uCpoiyx+0u+KzN9urjUdxR17Hku9a3TkxoPz3axtw47x57ITDrgUufGgpJblnLD9SOUdbaBGhpMuF1PM49cN8pqPeOBXn5w4wQvF05X+lX8TBZYfq1S+GQpfYvPpFkW+1WoY5vz0z+/WW/+HKucXBVh5Fk/A7JrXPoVacyCEto7UaSyXm429u1De5tfG/2yPu3jMm7ovsJfO66YBZax2Vhq+U65ePPKOdYd0c+AbLqydyKV3HCem/Ywq/wYXf3xAl19fMJR1yoRk4zXlB11CmPP/DPNTQx32B3sfGm8oeQTopLbNb92s/AR9bLbfID30gFL+ciltufr9RY7bBKfrwQ0awKAYbfZ/SyE9KLFEU57bd4CI28PikwEyPfoX+t17REBj84mW8rPtLbfV6fS3adnqXxVP27Lrsrl4+35JEb4LdxILa/P0r4lop61kQFUndej52fpwdXVPC/t98sTuGAY5xGBk9nlvNkxDEmZaemjvL6AWl5VOfptflHO8+nHDuv9s30PhtKSnsKHHUOz0WYotdxbxfsoXtqL2yJHpPDzNEkRAVlx5jkQ7aKo8TfVdH6P2G+Bzzs+lfcjNjXy0MHqQ/xcqCLg20en6HWz2KiJj83q9HbS5/nLanrx41FRXr2V16Vs36Kl54Wty1zyllofKGQBqtf2cqqebNqKc61BuXL7Oi1YmnsksMDZWLzVKF8uzKGnWrAeGDuOvl4yzNJWhQV+VQRUjjX7s/syrh7PodlhVtu5fBa4d1OCbv9ea1t9cAeVJHej54UrhF94T3penkMHk/T/LRfkPCSbs7ZQ9mcrLMe5Zdli+vHwSn58rJy2eC2f68NVCcKnVxLPM9uG/kIkezyh9Oh4Nl1bMc7oh4sAT1ea4zIPVzKP8FQGjLNZws6CfZbix8TBeN2u+sv81vMNtEv//ztY10CrtPRB1QbaUPXA4a+Sr/0vXDUCmgzgmdRwNsvwuX4kU8/HUW6i3lYL9kcylfl1QATc/yKH5hz/Wm+vHo8eWFn+/hVquHfRIgJMP9YmhKo29NVtXenoHGGP21FNtbmTKLvG/bjdRIAM9CrqeF9qbUbb6u0iQNhjqOGO+H5pL1d/PMXTzPIyR11r7K320iSlXKLsnzL16n9RRndhY2X1VeGkXvO4LVQvZ1a+JrYBW6giAnb/8Bcqa/p3sfHb7Lu0Pa+P8Ur2kgxTvL2Jbae8NKOLWZ558z9EH/03UVnz/9XtkVTw8q+0ueKQ4Td15CTuJwXqRyU/aeW/UpQiAj6//ycqf/WfFBMuyrzfCNFO9rPk+j/Qsef/LMrJNbyu+7w6Lf0fox3DTaSUZw1z2FTc2kh8LgIYFwrNoC9FAeP2IvnhtNGVbSfstKsiIHJEBg8chctFIFZFwLcvRb5tETDUCHwqLc3iH1oGrR+vTOfp96+swuPS93rA8jhXAliA7a6lGUWH6Jv8/tTE+xpBLU928PkZvmnLLCLg+p44MfbTnTxt+maxKOtzSTy4n2ZoHw5PZD96cGoMb88+tF/9+qyeF8c1Uku31qmBWGyOJPuR6eXHFwwRcKFoERWd2u3wqdGOk4kLdowPTyfpdeLcvv5+s8U3cmmWmJ+Wb9bnzkRFvX5cbAXk3PcXxLwiR1L+SPP8zWbtYmdwgeT5dD8Vr5VfjoIp40ZRaK94io80baoIOLJ7F1VOiOb5pjLrUhcLxizlQVILiCywNlVst/ioqCLAIGIINRXMcfiK/sU8cmelcdS6Rr2OiQDue2yp4S/ndas4hwa59Cv7VlchmAjoz+pC+whxEjuBGgvFZ8MuUlQR0FTxucVHpts2fU4l+t+BnWMWmLq6zMOdyfzL7nxhIS/LwO8mAhoeVLcqAs6qQV4LxFUbNPtXhXTeCPCinZ3zW6O0OvY5VAN/ayJAyTtEwNtf/Tb86iSV3WygZN5eCfw2EVBxq+ENImAALVP6vF26mB5q9j1fNNDD26VU08p8mAgw52xdCbh/he1MKvyc43lo0opttG3bNsMm6bgIGEpX727nKwGeRVve4mo3SQtGdTzPgtKm5p950N7y1X/xvHlVX24RAUeuiO+e8pZ/EWljM0+XPvyraJP6Nc0azzaV66YH1RVUWFtAW7/6I8+3e++R0KHK/K7wNE/rL+tgBiUc/3vatCPRCNqzbvyF5o4X7Y7eEN+F5S1CKJS//FuefvrNf4hzM6KSMkaLTe+MoK8F+eyqbTy/r2IOfXz9L7QxO1UrD9F8/sTtY8/9K81IZu3mU/nzl8Y8T7sEdG/BUofNqNP8N2S2fsvAL7cDVBHA6OhKgBssKLIg1tGVgB9qJjtsMgALxJWshNleaemRvEV08nqF4WcXAWobdhXO5jf98EEeEC1L5zYRwAMgb8/m3s8IknLsY3fMccQ8YyhRu5L2pK+ghVXiA1/zpNoYe0+Cfgz6MZkBvpSn6u0AJoiWnivVREu84xhY8GbHKEVTS8sJnlYv6cpTeUzqbQV5/s2VBYG6csLGZjb7+XNbCXBDFQF3lGBpBLry7Twoy/K7iICHWh+9XXwZN0rNlQA5JzZmgTa2DP7uIsCc774Bzn4Z9nkwEaDWJc9dSZczulvGkKgi4Ml29qViFQESw68D5E+eTNO1dGuUGWzdRMCqE3dpQSsiYJd2xZsVJcq5tQ20gdsfEAvw7LZAQ437DpgNV/Ppq8LpFJV5hEoWSHtrIiDK9HGIAP3/7y1WAljwbfh1LZ1c1Zd+fVVsjCXsVhHA0s2rWhMBXah8ubwyjabSxR6+JP+w4VdaoP+15TypdGQlgK2YyJUANoZo66uVAPFdOPn4cUdda3irmViJIG/dRfKWZ1HS+X+lxNnafKJOU6lyRWwXAbN6iFQG0KxsfR8YfSVgdCUL9laf8te/1erWUuT6p455tIa3hl3dh2vzq6G6KrHNPBMBqg/rX8KCN7Mt1lcq5di7ij4W/vpKQNyh31na8TpNBKjiaVujWS99mAiwz5HhdlUfUCsBEwen0eWi3/M0Rre1KQI0vFeKbLYQ8tabvyDIlo1LjyziAT9/rHgmoEgLzI2vL9KrH7bpPmWUp9kYe1YOcowhfC5S8+MjdOrSXpoR7aHUVC1gPtujpaY/87lQukTzEVdtrJz68SRqvrWbVumKiomRR7d2UF6J+PKp/u48tbyq1MZeRePDRHC88uA8VesBmvmwsVJXrqWZLB3Tw0UEiONMTR1tCBtPbDo1PtpP9Q9O055RYlnX+y3bvS+G6h+KlQnZP8/zK3U3EXCRjm9NoedPKhy3A6QPO5fs3BSdEufTTQSwFYbJ2vzNWw8xfM7snB8Zr82vex96zs6X5iPvk8eu2Uyvnh/jPuy2Qr/RQ+jE3bPcR86bH9cb/lElqgjoNnKqEXhrF4mg9uzAfMqYOIEea/b0gT3bJQJmTk2jw2tW090NU2ltQqRhb6rIdvgahPbkY+dpY989JoPsThofP5Au79xKnyfHuYoAtkrw/b4ldPyz+c4+ddg85ArDMI9TBHg83YmtKGSkTaHner9J8bHa2LH0aP04nrqJgJvFOdR4bAPlzv3kjc8wVLfxdzhfJpaAr1ZV8TRl0TY6dP4WlW5TrjivW28bsJTVsbRyA1slYlf0DZRzpNLiw4L5A63fml1iJckOEwksmFdVXacF3Jai9XuIGr4s1VLx2xO3zh/S+q2wBtT2iICEFTRNDzhuyKvu+19W0dX8adR10gr9mO6Zx62LAOYrRQCr++Kre3S9RARtVper2dgKACsvKLnBg/Dxr1tfmXATAfe+OMr7Zkzp6+HpzsIanmbNHUWeyPG8v6qjuXT93tuJgLY+i2xF8bPspWI1wLBPpKxhTl+1z/WrVvG0rnIHX7bf5P13XicDH9v9ND61loayVN8B1S4CWDpYqz/+9M/66sFwKnvxf2jZ9X/S6v5N9xFX05vqRSoYSbvTuznmpc5vo5zfmT0Ux3Zi1cZic5I+uc0/U+mLP9Ks4w3G7Qm7CGDpcDa/5382Aj1b0p+dd4k2ffWaPBEDKX7J1zRa84mWy/zJ1bwd22F285ki6jIonaZc/rNlbHWearnv0iNtrnZ4vXKV2InPRUBHCek23nFw74p6ZasGShC4sP8BvB3QfmSA9yW+/hx2FC4KFOz1vmb8lpMO2y+CFqDf53G70aH/gTnWB9YCjknbKcJu60S09nZA3mrr9utu1Gl+3iL9uSQbASMCAADtRy7r184d4qh7V+T90GBjR1W9wxaseOvbf5tEsviwuJ8eqGTsPuewAT89EwAAAACAwAcrAQAAAECQ4hcR4Kt+fklWHzjpuP8CAhd/3BsHgYl1d9GuPK/e62Xlsl3zHO0AAE78cjvg3UVAJH+Qr6gqW3m6/Rzl5S3lZfbO9A7vKVrkaGflQQcfBjx62UtzYq22rxZ2p7/jG6uIMgs6FfPS6LvStws+rT1xbn/PXWXh6k10OcMsX3uPAc9nwTW0D9XOHuy024hKmknPs5Ic9jcTQtWTwlzsb0uY5Ty/D77Tzm/m/OV0Qt+cqDV+V7mYGu6a/39tE80/K8fO5CifobPGZ6ivVt57vZJmONrZCeNvhBTkLdJS9UlwKy0t1h3JWpqKjDd17L7vROR08taar8FJrtgEe9iwNVR/RrzmBQBoHb+sBLwzCTPpVYP1nfGWFvE6kqd7CrU8z+mwCBi9ehn/Ehyq7EvABceSWLp5SLwm6LYCcG1uV/qDLgLSl66l67PtO76F0OPibKpdNJqX9+/IoS3cHmYJ8FYREEJPNBFxZ8tUw+en3Ex6pPVzdKLZf49Zy6gmXeSZIFDf97bOwSSkWxyvv7lukj5uDv2wbjR/Ra00rTe3zZkzmxpLthk7XYn37ncaZXWcppIFot/uQ3gfa/q7B93o8Z8abdmx58eJwM7K7NiEXwTPM5s8zpDIWN5vL0UErFo4j346br4ed2fJQDp1SL6OZ+4KeCTbPKesz7F62kO33TiwlR+n2s/TcnHe1XbZQyINEcCOk9nkcbJ8Y9Fs3u7aUvE3Tk2fpv3dthvjMJ/zaSI1z08o/XTMfTMsxlPNt8+kuUY7ZpuV8TEfR5YZvz25gOpvvIUIGD2XfnPf+nluadFfE+r5EbU82dkuEXDxsfMzNHg+EwQX6capaXq/6ls4Qgy0vDA3sDF8tDGbXl+k21XTaGZVMbetPy12oBR+YfS06YLR7/UXos++ny6m5udit8k1x72ue4pELzTf25eon+Ohiw/StgzbZlAAAP+IAF/0E95rCP8CuFaUwsuGCNDzHRUBUlzIqyP1ffc3iQCV0/k59LHNJr+wxyxYTd8uimuXCFC/5KVPU9lKR52da2+o40SM0QLWXJs9whib9c3mKa8+7WNZ5mWrazwk3lutOJBj2cfdIIy93x1OuzXh4omS7yFrhA+ziAD7ccq0e+ZCLgLY/L5ZEGup48KBnafwIcpqQbjl/DKR9MP6RMPO+pF1TaXiqvRFttgwiv0d1f7ZRi5cBIT0thyn0V6Za9dJ86h+unjnmAVy7hOVpo0pxrOftzdxKX8X3Vtrrn40lZvC510I7yM2vqrPF9sEGyJAz7dHBLzQPx8yyLP89xc+Ev1/Yn7OHSsBNlEgbSz9SU9ZkOd1Y+fTi+tzjfqJBXn0kO2OqbcZrKXzzpbQ6RkeOtraZ9Nl19G2PscAgAAWARL5xWCIAC2YtDQf6bAIeHpFBCK2oY3aP6O9IoAFGhmgJPJLXy5nd1gE6FfcbwoibYoALRg91zeKMYkw+ma4rSiwVN1tT9rUftQ2diEkCKEsbXxPlxE8oBt2uwiwHac8L/L8LVuTRZXJ1jk0la2h55tTKHPoNKPuQkEOTVDGZ8dVMsJaNuasn2MpIJwiQL8doM3Vfm6En7mzJFudcfiw8/6WtzLKcrO5mGIrG9eLzbFK1or+7f4dwfwM6SIgfAy1NB1qlwiofWp+Psx+nHtvOESAbSVA+Fg3LDFEgK1/z8wNXBRYbDqtrQR0XS62MVZp63MMAAjQZwLSSguNL5nXz3K5zfji0ZcOmQhw+zJSYSLA7sN2xWP5xnvreLnnvM/EOC/2GyJgavYZOnfgzQ8Wsas3NQAkz1piDQihfXn+mRaMWSoEgYee6D4sUKlt3kYEhA8UgfpNPvVF2YoPWwUQefWeN9ttTthFcJM+LDjKoLx/+3bdLsqnD1iP2w1ZJ9Nrxjg5+vE5RcApvd+aT9KNQCrbfDVfPEvA8nt7qOcmmpqKlyhjh7nOTdrksdtFQO/xM3h9Y8E8w8d+nDKvBvnH+pK9usLCkH9rTsQYx3zcYLdC7GO11e5N7/N/cuqY+Rl6IvoxP0NiJ04mAuyfDzfsPvebzG2qpQ9bUVNt9jYyL4O7XOpX+0jZJ55fsLdRfdjtFbfgbretKrpEJetGGOWF+Vdo7wznsQEQ7PhlJeCXQv3SeHxeWYpuD9rV3yH96pKxrfiC44sFBBbXtCvnAS72YMFb1vZPR78t6mfo0emxjvpAwLq7qHg7QP5ML4OVTx9YbmmTf8VLfVz6AiDY8YsI8FU/74MtdSdcrjYACHz2fxrlsAF32LapdhsAIEBvBwAAAADA//hlJQAAAAAAgY9fRICv+nlvhA+gxl9vctoVNhRfIq/X/IGMC198SWcPKk+/AwAAAJ2MABUBvtkxMLv+BL1uLKHymgP07KrYxMadaLp7abZRdnu10Fvn/D3mC/VeY6MYAAAAoLMRoM8ExGnB/rzFZm4WxARCebtEgPqwn8zLtwaMuoSZ4sHARvFb2O6vHibStG7O/j0j19GFfTOM8uHLXupt9wEAAAACFL+sBPgCX+wY2C4RwOlriACGcyVgFsW69M/s3uLW9/wHAAAAAhm/iABf9cMwbwcIEZCwfRe9vLO8wyLgx2qx5enbiQAPFSwV++2rpG2rpEPzejjsAAAAQGcgIG8H+GrHwPCEdKN+WX9hU9uwVYGKB2aZ3WZgPj3nrXX067ZxkN226mjrO7gBAAAAgYZfVgJ+KewBXtgH0+sfPnf4vjVdRliCPsv3DbX6HL3iFAoAAABAoOIXEeCrfnxGZE+qLU932n3Muepqhw0AAAAIVIJDBAAAAADAQUA+EwAAAAAA/+OXlQAAAAAABD5+EQG+6qctHp/6vcPmCz79qBdd/fGCww4AAAB8SATk7YCtm3+iZi3AS5ht9tL7lrJa33yqWdhKbht1ps8LS7tn2adFueJ7x7gqdhHAfop0x4RIhx8AAADQWfHLSoAvuGu7ym8+9VLke++iR2sW87x9JcAuAux5Xi4Q7/7b7XbsIgAAAAD40PCLCPBFP04R4AzaDhFQesfha28HEQAAAAAIAlIEMAGgLuEz4pL1ZXyNJN02ato1i99LPc/EAVsVkHUWUdAOEcAEgETaztXjx4EAAAB8WATkMwGBiH2LYAAAAKCz45eVgA+RauwGCAAA4APDLyLAV/0AAAAAwH/45XYAAAAAAAIfv6wE4P45AAAAEPhABAAAAABBCkQAAAAAEKT45ZkAJgJ6dXHaAQAAABA4YCUAAAAACFIgAgAAAIAgxee3A9imOtPiuzjsAAAAAAgs/LISAAAAAIDAByIAAAAACFJ8fjsAAAAAAJ0DiAAAAAAgSMHtAAAAACBIgQgAAAAAghTcDgAAAACCFKwEAAAAAEHK/wejSWTJy1Hj0AAAAABJRU5ErkJggg==>
