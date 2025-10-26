#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/compatibility.hpp> // For glm::lerp
#include <glm/common.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model2.h>

#include <iostream>
#include <cmath>
#include <vector>
#include <string> // For UI text
#include <sstream> // For UI text formatting
#include <iomanip> // For std::setprecision

// --- IMGUI INCLUDES REMOVED ---

// callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double ypos);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// --- Car state ---
glm::vec3 carPos = glm::vec3(0.0f, 0.0f, 0.0f);
float carYaw = 0.0f;
float carForwardSpeed = 0.0f;
float carVerticalSpeed = 0.0f; // NEW: For jumping
const float CAR_MAX_SPEED = 6.0f;
const float CAR_ACCEL = 10.0f;
const float CAR_BRAKE = 12.0f;
const float CAR_REVERSE_MAX = -3.0f;
const float CAR_TURN_SPEED = 90.0f;
const float CAR_FRICTION = 5.0f;
const float GRAVITY = -18.0f; // NEW: Strong gravity for a gamey feel
const float JUMP_FORCE = 8.0f; // NEW: How high the jump is
float teleporterCooldown = 0.0f; // NEW: Cooldown for teleporters
float timeSlowTimer = 0.0f; // NEW: Cooldown for time slow

// camera follow
float CAM_DISTANCE = 8.0f;
const float CAM_HEIGHT = 2.2f;
const float CAM_LOOKAT_HEIGHT_OFFSET = 0.9f;
glm::vec3 cameraPos_current = glm::vec3(0.0f, CAM_HEIGHT, CAM_DISTANCE);
const float CAMERA_POSITION_FOLLOW_SPEED = 7.0f;
const float CAMERA_ROTATION_FOLLOW_SPEED = 5.0f;

// --- GAME STATE LOGIC ---
enum class GameState {
    // START_MENU removed, we will just start playing
    PLAYING,
    GAME_OVER
};
// Start in the PLAYING state
GameState currentState = GameState::PLAYING;
float gameTimer = 0.0f;
int score = 0;
int totalItems = 0;
// --- END GAME STATE ---

// --- ASSIGNMENT 3: ITEMS & COLLISION ---
struct GameItem {
    glm::vec3 position;
    bool isAlive;
};
std::vector<GameItem> gameItems;
unsigned int cubeVAO, cubeVBO;

const float PLAYER_RADIUS = 1.0f;
const float ITEM_RADIUS = 0.5f;
float gridSize = 40.0f;

// --- NEW: OBSTACLES ---
std::vector<glm::vec3> staticObstacles; // Renamed from obstacles
const float OBSTACLE_RADIUS = 0.5f;

// NEW struct for moving obstacles
struct MovingObstacle {
    glm::vec3 position;  // Current position
    glm::vec3 startPos;
    glm::vec3 endPos;
    float speed;
    float t; // interpolation factor 0 to 1
    bool forward; // direction
};
std::vector<MovingObstacle> movingObstacles;
// --- END NEW ---

// --- NEW: BOOST PADS ---
std::vector<glm::vec3> boostPads;
const float BOOST_PAD_RADIUS = 1.5f;
// const float BOOST_SPEED_MULTIPLIER = 1.8f; // REMOVED: No longer speed boost
// --- END NEW ---

// --- NEW: TELEPORTERS ---
struct TeleporterPair {
    glm::vec3 posA;
    glm::vec3 posB;
};
std::vector<TeleporterPair> teleporterPairs;
const float TELEPORTER_RADIUS = 1.5f;
// --- END NEW ---

// --- NEW: TIME SLOW PADS ---
struct TimeSlowPad {
    glm::vec3 position;
    bool isAlive;
};
std::vector<TimeSlowPad> timeSlowPads;
const float TIME_SLOW_RADIUS = 1.5f;
const float TIME_SLOW_DURATION = 5.0f;
const float TIME_SLOW_FACTOR = 0.3f; // Game runs at 30% speed
// --- END NEW ---

// --- END ASSIGNMENT 3 ---

// Cube vertices (unchanged)
float cubeVertices[] = {
    // positions           // normals
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

// --- NEW FUNCTION: ResetGame ---
// Resets all game variables to their starting state
void ResetGame() {
    // Reset car
    carPos = glm::vec3(0.0f, 0.0f, 0.0f);
    carYaw = 0.0f;
    carForwardSpeed = 0.0f;
    carVerticalSpeed = 0.0f; // NEW: Reset vertical speed

    // Reset camera
    cameraPos_current = glm::vec3(0.0f, CAM_HEIGHT, CAM_DISTANCE);

    // Reset score and timer
    score = 0;
    gameTimer = 0.0f;

    // Respawn items
    gameItems.clear();
    gameItems.push_back({ glm::vec3(5.0f, ITEM_RADIUS, 5.0f), true });
    gameItems.push_back({ glm::vec3(-5.0f, ITEM_RADIUS, 8.0f), true });
    gameItems.push_back({ glm::vec3(0.0f, ITEM_RADIUS, -10.0f), true });
    gameItems.push_back({ glm::vec3(8.0f, ITEM_RADIUS, -2.0f), true });
    gameItems.push_back({ glm::vec3(15.0f, ITEM_RADIUS, 15.0f), true });
    gameItems.push_back({ glm::vec3(-10.0f, ITEM_RADIUS, -12.0f), true });

    totalItems = gameItems.size();

    // --- Spawn Obstacles ---
    staticObstacles.clear(); // Renamed
    // Place some static obstacles
    staticObstacles.push_back(glm::vec3(2.0f, OBSTACLE_RADIUS, -3.0f));
    staticObstacles.push_back(glm::vec3(10.0f, OBSTACLE_RADIUS, 10.0f));
    staticObstacles.push_back(glm::vec3(7.0f, OBSTACLE_RADIUS, 13.0f));

    // --- NEW: Spawn Moving Obstacles ---
    movingObstacles.clear();
    movingObstacles.push_back({
        glm::vec3(-4.0f, OBSTACLE_RADIUS, 6.0f), // startPos
        glm::vec3(-4.0f, OBSTACLE_RADIUS, 6.0f), // currentPos
        glm::vec3(4.0f, OBSTACLE_RADIUS, 6.0f),  // endPos
        2.0f, // speed
        0.0f, // t
        true  // forward
        });
    movingObstacles.push_back({
        glm::vec3(-12.0f, OBSTACLE_RADIUS, -5.0f), // startPos
        glm::vec3(-12.0f, OBSTACLE_RADIUS, -5.0f), // currentPos
        glm::vec3(-12.0f, OBSTACLE_RADIUS, 5.0f), // endPos
        3.0f, // speed
        0.0f, // t
        true  // forward
        });
    // --- END NEW ---

    // --- NEW: Spawn Boost Pads ---
    boostPads.clear();
    boostPads.push_back(glm::vec3(0.0f, 0.05f, 5.0f)); // y is slightly above ground
    boostPads.push_back(glm::vec3(8.0f, 0.05f, 8.0f));
    boostPads.push_back(glm::vec3(-8.0f, 0.05f, -8.0f));
    // --- END NEW ---

    // --- NEW: Spawn Teleporter Pads ---
    teleporterPairs.clear();
    teleporterPairs.push_back({
        glm::vec3(15.0f, 0.05f, -15.0f), // posA
        glm::vec3(-15.0f, 0.05f, 15.0f)  // posB
        });
    teleporterCooldown = 0.0f; // Reset cooldown
    // --- END NEW ---

    // --- NEW: Spawn Time Slow Pads ---
    timeSlowPads.clear();
    timeSlowPads.push_back({ glm::vec3(-5.0f, 0.05f, -5.0f), true });
    timeSlowPads.push_back({ glm::vec3(10.0f, 0.05f, 0.0f), true });
    timeSlowTimer = 0.0f;
    // --- END NEW ---
}


int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sliding Puppy Game", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // --- Set cursor to disabled for gameplay ---
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // OpenGL state
    glEnable(GL_DEPTH_TEST);

    // shader
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");

    // model
    Model carModel(FileSystem::getPath("resources/objects/dog/dog.obj"));
    Model gummyModel(FileSystem::getPath("resources/objects/car/car.obj")); // NEW: Load the gummy model

    // ground plane grid (unchanged)
    std::vector<float> gridVerts;
    const int GRID_DIV = 40;
    for (int i = -GRID_DIV; i <= GRID_DIV; ++i) {
        float t = (float)i / (float)GRID_DIV * gridSize;
        gridVerts.push_back(-gridSize); gridVerts.push_back(0.0f); gridVerts.push_back(t);
        gridVerts.push_back(gridSize);  gridVerts.push_back(0.0f); gridVerts.push_back(t);
        gridVerts.push_back(t); gridVerts.push_back(0.0f); gridVerts.push_back(-gridSize);
        gridVerts.push_back(t); gridVerts.push_back(0.0f); gridVerts.push_back(gridSize);
    }
    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(float), gridVerts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // Setup Item VAO (unchanged)
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // --- IMGUI SETUP REMOVED ---


    // Initialize game state
    ResetGame(); // Call this to populate items, reset score, etc.
    currentState = GameState::PLAYING; // Start immediately
    std::cout << "--- Game Started! ---" << std::endl;
    std::cout << "Collect all " << totalItems << " red cubes." << std::endl;
    std::cout << "Avoid the gray obstacles!" << std::endl;
    std::cout << "Hit the purple pads to jump!" << std::endl; // New message
    std::cout << "Hit the cyan pads to teleport!" << std::endl; // NEW message
    std::cout << "Hit the yellow pads to slow time!" << std::endl; // NEW message
    std::cout << "Controls: W, A, S, D to drive. ESC to quit." << std::endl;


    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // timing
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // --- NEW: Time Slow Logic ---
        float actualDeltaTime = deltaTime; // Store real time
        if (timeSlowTimer > 0.0f) {
            timeSlowTimer -= actualDeltaTime; // Deplete using real time
            deltaTime *= TIME_SLOW_FACTOR; // Scale the global deltaTime for game logic
        }
        // --- END NEW ---

        // input
        processInput(window); // This will now use the scaled deltaTime

        // --- Calculate carForward vector (needed for both logic and camera) ---
        float rad = glm::radians(carYaw);
        glm::vec3 carForward = glm::normalize(glm::vec3(sin(rad), 0.0f, -cos(rad)));

        // --- GAME LOGIC (STATE-DEPENDENT) ---
        if (currentState == GameState::PLAYING)
        {
            // Update timer
            gameTimer += deltaTime;

            // NEW: Update teleporter cooldown
            if (teleporterCooldown > 0.0f) {
                teleporterCooldown -= deltaTime;
            }

            // --- NEW: Update Moving Obstacles ---
            for (MovingObstacle& obs : movingObstacles)
            {
                // Calculate total distance once, or store it
                float totalDistance = glm::distance(obs.startPos, obs.endPos);
                if (totalDistance > 0.001f) // Avoid division by zero
                {
                    float distanceToTravel = obs.speed * deltaTime;
                    float deltaT = distanceToTravel / totalDistance;

                    if (obs.forward) {
                        obs.t += deltaT;
                        if (obs.t >= 1.0f) {
                            obs.t = 1.0f;
                            obs.forward = false;
                        }
                    }
                    else {
                        obs.t -= deltaT;
                        if (obs.t <= 0.0f) {
                            obs.t = 0.0f;
                            obs.forward = true;
                        }
                    }
                    obs.position = glm::lerp(obs.startPos, obs.endPos, obs.t);
                }
            }
            // --- END NEW ---

            // --- Vehicle Physics & Vectors ---
            // Apply gravity
            if (carPos.y > 0.0f || carVerticalSpeed > 0.0f) { // Check if in air
                carVerticalSpeed += GRAVITY * deltaTime;
            }

            // Update position
            carPos += carForward * carForwardSpeed * deltaTime; // Update XZ
            carPos.y += carVerticalSpeed * deltaTime; // Update Y

            // Ground collision
            if (carPos.y < 0.0f) {
                carPos.y = 0.0f;
                carVerticalSpeed = 0.0f;
            }

            // --- COLLISION DETECTION ---
            // 1. Player-Item Collision
            for (GameItem& item : gameItems)
            {
                if (item.isAlive)
                {
                    float distance = glm::distance(carPos, item.position);
                    if (distance < (PLAYER_RADIUS + ITEM_RADIUS))
                    {
                        item.isAlive = false;
                        score++; // Increase score!
                        // --- Print score update to console ---
                        std::cout << "Score: " << score << " / " << totalItems << std::endl;
                    }
                }
            }

            // --- 2. Player-Obstacle Collision ---
            // --- 2a. Static Obstacles ---
            for (const glm::vec3& obsPos : staticObstacles) // Renamed
            {
                // Check 2D distance on XZ plane
                float distance = glm::distance(glm::vec2(carPos.x, carPos.z), glm::vec2(obsPos.x, obsPos.z));

                // Check if they are vertically overlapping (AABB check on Y)
                // Assumes player "height" is roughly PLAYER_RADIUS
                bool yOverlap = (carPos.y < (obsPos.y + OBSTACLE_RADIUS)) && ((carPos.y + PLAYER_RADIUS) > (obsPos.y - OBSTACLE_RADIUS));

                if (distance < (PLAYER_RADIUS + OBSTACLE_RADIUS) && yOverlap)
                {
                    // Collision detected!
                    carForwardSpeed = 0.0f; // Stop the car

                    // Push the player back out of the obstacle (XZ only)
                    glm::vec2 dirFromObs2D = glm::normalize(glm::vec2(carPos.x - obsPos.x, carPos.z - obsPos.z));

                    // Avoid division by zero if perfectly centered (rare)
                    if (glm::length(dirFromObs2D) < 0.001f) {
                        dirFromObs2D = glm::vec2(1.0f, 0.0f); // Push in a default direction
                    }

                    // Set carPos X and Z to be just outside the collision radius
                    glm::vec2 newPos2D = glm::vec2(obsPos.x, obsPos.z) + dirFromObs2D * (PLAYER_RADIUS + OBSTACLE_RADIUS);
                    carPos.x = newPos2D.x;
                    carPos.z = newPos2D.y;
                    // carPos.y remains unchanged, letting gravity handle it
                }
            }

            // --- 2b. Moving Obstacles ---
            for (const MovingObstacle& obs : movingObstacles)
            {
                // Check 2D distance on XZ plane
                float distance = glm::distance(glm::vec2(carPos.x, carPos.z), glm::vec2(obs.position.x, obs.position.z));

                // Check if they are vertically overlapping (AABB check on Y)
                bool yOverlap = (carPos.y < (obs.position.y + OBSTACLE_RADIUS)) && ((carPos.y + PLAYER_RADIUS) > (obs.position.y - OBSTACLE_RADIUS));

                if (distance < (PLAYER_RADIUS + OBSTACLE_RADIUS) && yOverlap)
                {
                    // Collision detected!
                    carForwardSpeed = 0.0f; // Stop the car

                    // Push the player back out of the obstacle (XZ only)
                    glm::vec2 dirFromObs2D = glm::normalize(glm::vec2(carPos.x - obs.position.x, carPos.z - obs.position.z));

                    // Avoid division by zero if perfectly centered (rare)
                    if (glm::length(dirFromObs2D) < 0.001f) {
                        dirFromObs2D = glm::vec2(1.0f, 0.0f); // Push in a default direction
                    }

                    // Set carPos X and Z to be just outside the collision radius
                    glm::vec2 newPos2D = glm::vec2(obs.position.x, obs.position.z) + dirFromObs2D * (PLAYER_RADIUS + OBSTACLE_RADIUS);
                    carPos.x = newPos2D.x;
                    carPos.z = newPos2D.y;
                    // carPos.y remains unchanged, letting gravity handle it
                }
            }
            // --- END NEW ---

            // --- NEW: 3. Player-Boost Pad Collision ---
            for (const glm::vec3& padPos : boostPads)
            {
                // We only check XZ distance for a flat pad
                float distance = glm::distance(glm::vec2(carPos.x, carPos.z), glm::vec2(padPos.x, padPos.z));
                // Check if on the ground
                if (distance < (PLAYER_RADIUS + BOOST_PAD_RADIUS) && carPos.y < 0.1f)
                {
                    // Apply jump!
                    carVerticalSpeed = JUMP_FORCE; // Set vertical speed
                }
            }
            // --- END NEW ---

            // --- NEW: 4. Player-Teleporter Pad Collision ---
            if (teleporterCooldown <= 0.0f) // Only check if cooldown is over
            {
                for (const auto& pair : teleporterPairs)
                {
                    // Check pad A
                    float distA = glm::distance(glm::vec2(carPos.x, carPos.z), glm::vec2(pair.posA.x, pair.posA.z));
                    if (distA < (PLAYER_RADIUS + TELEPORTER_RADIUS) && carPos.y < 0.1f)
                    {
                        carPos = pair.posB; // Teleport to B
                        carVerticalSpeed = 0.0f; // Reset vertical
                        teleporterCooldown = 1.0f; // Set 1-second cooldown
                        break; // Stop checking
                    }

                    // Check pad B
                    float distB = glm::distance(glm::vec2(carPos.x, carPos.z), glm::vec2(pair.posB.x, pair.posB.z));
                    if (distB < (PLAYER_RADIUS + TELEPORTER_RADIUS) && carPos.y < 0.1f)
                    {
                        carPos = pair.posA; // Teleport to A
                        carVerticalSpeed = 0.0f; // Reset vertical
                        teleporterCooldown = 1.0f; // Set 1-second cooldown
                        break; // Stop checking
                    }
                }
            }
            // --- END NEW ---

            // --- NEW: 5. Player-TimeSlow Pad Collision ---
            for (TimeSlowPad& pad : timeSlowPads)
            {
                if (pad.isAlive)
                {
                    float distance = glm::distance(glm::vec2(carPos.x, carPos.z), glm::vec2(pad.position.x, pad.position.z));
                    if (distance < (PLAYER_RADIUS + TIME_SLOW_RADIUS) && carPos.y < 0.1f)
                    {
                        pad.isAlive = false; // Consume
                        timeSlowTimer = TIME_SLOW_DURATION; // Activate
                    }
                }
            }
            // --- END NEW ---


            // 6. Player-Scene Collision (was 5)
            if (carPos.x > gridSize) { carPos.x = gridSize; carForwardSpeed = 0; }
            if (carPos.x < -gridSize) { carPos.x = -gridSize; carForwardSpeed = 0; }
            if (carPos.z > gridSize) { carPos.z = gridSize; carForwardSpeed = 0; }
            if (carPos.z < -gridSize) { carPos.z = -gridSize; carForwardSpeed = 0; }

            // 7. Win Condition Check (was 6)
            if (score == totalItems)
            {
                currentState = GameState::GAME_OVER;

                // --- Print final message to console ---
                std::cout << "\n--- You Win! ---" << std::endl;
                std::stringstream ss;
                ss << std::fixed << std::setprecision(2) << gameTimer;
                std::cout << "Final Time: " << ss.str() << "s" << std::endl;
                std::cout << "Press ESC to quit." << std::endl;
            }
        }
        // --- END GAME LOGIC ---


        // clear
        glClearColor(0.08f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // --- 3D RENDER ---
        ourShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        ourShader.setMat4("projection", projection);

        // Camera logic (unchanged)
        glm::vec3 lookTarget = carPos + glm::vec3(0.0f, CAM_LOOKAT_HEIGHT_OFFSET, 0.0f);
        glm::vec3 desiredCameraPos = lookTarget - (carForward * CAM_DISTANCE) + glm::vec3(0.0f, CAM_HEIGHT - CAM_LOOKAT_HEIGHT_OFFSET, 0.0f);
        float posFollowSpeed = glm::clamp(deltaTime * CAMERA_POSITION_FOLLOW_SPEED, 0.0f, 1.0f);
        cameraPos_current = glm::lerp(cameraPos_current, desiredCameraPos, posFollowSpeed);
        glm::vec3 currentDir = glm::normalize(lookTarget - cameraPos_current);
        glm::vec3 desiredDir = glm::normalize(lookTarget - desiredCameraPos);
        float rotFollowSpeed = glm::clamp(deltaTime * CAMERA_ROTATION_FOLLOW_SPEED, 0.0f, 1.0f);
        glm::vec3 smoothedDir = glm::normalize(glm::lerp(currentDir, desiredDir, rotFollowSpeed));
        glm::mat4 view = glm::lookAt(cameraPos_current, cameraPos_current + smoothedDir, glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("view", view);

        // Lighting (unchanged)
        ourShader.setVec3("viewPos", cameraPos_current);
        glm::vec3 lightPosition = glm::vec3(10.0f, 15.0f, 10.0f);
        ourShader.setVec3("lightPos", lightPosition);
        ourShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);

        // Grid (CHANGED to look like ice)
        ourShader.setMat4("model", glm::mat4(1.0f));
        ourShader.setInt("has_diffuse_texture", 0);
        ourShader.setVec3("material_diffuse_color", 0.8f, 0.95f, 1.0f); // Light blue
        ourShader.setVec3("material_specular_color", 1.0f, 1.0f, 1.0f); // Shiny white
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, (GLsizei)(gridVerts.size() / 3));
        glBindVertexArray(0);

        // --- NEW: Draw Boost Pads (draw first, so they are under items/obstacles) ---
        ourShader.setInt("has_diffuse_texture", 0);
        ourShader.setVec3("material_diffuse_color", 0.6f, 0.1f, 0.8f); // Bright Purple
        ourShader.setVec3("material_specular_color", 0.5f, 0.5f, 0.5f);
        glBindVertexArray(cubeVAO);
        for (const glm::vec3& padPos : boostPads)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, padPos);
            // Scale it to be flat and wide
            model = glm::scale(model, glm::vec3(BOOST_PAD_RADIUS * 2.0f, 0.1f, BOOST_PAD_RADIUS * 2.0f));
            ourShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);
        // --- END NEW ---

        // --- NEW: Draw Teleporter Pads ---
        ourShader.setInt("has_diffuse_texture", 0);
        ourShader.setVec3("material_diffuse_color", 0.2f, 0.7f, 0.9f); // Bright Cyan
        ourShader.setVec3("material_specular_color", 0.8f, 0.8f, 0.8f);
        glBindVertexArray(cubeVAO);
        for (const auto& pair : teleporterPairs)
        {
            // Draw A
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pair.posA);
            model = glm::scale(model, glm::vec3(TELEPORTER_RADIUS * 2.0f, 0.1f, TELEPORTER_RADIUS * 2.0f));
            ourShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Draw B
            model = glm::mat4(1.0f);
            model = glm::translate(model, pair.posB);
            model = glm::scale(model, glm::vec3(TELEPORTER_RADIUS * 2.0f, 0.1f, TELEPORTER_RADIUS * 2.0f));
            ourShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);
        // --- END NEW ---

        // --- NEW: Draw Time Slow Pads ---
        ourShader.setInt("has_diffuse_texture", 0);
        ourShader.setVec3("material_diffuse_color", 0.9f, 0.9f, 0.1f); // Bright Yellow
        ourShader.setVec3("material_specular_color", 0.8f, 0.8f, 0.8f);
        glBindVertexArray(cubeVAO);
        for (const auto& pad : timeSlowPads)
        {
            if (pad.isAlive)
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, pad.position);
                model = glm::scale(model, glm::vec3(TIME_SLOW_RADIUS * 2.0f, 0.1f, TIME_SLOW_RADIUS * 2.0f));
                ourShader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
        glBindVertexArray(0);
        // --- END NEW ---


        // Draw Items (only if playing)
        // We can draw items even in GAME_OVER state, they will just all be gone
        if (currentState == GameState::PLAYING)
        {
            // --- MODIFIED: Draw gummy model instead of cube ---
            // ourShader.setInt("has_diffuse_texture", 0); // No longer needed
            // ourShader.setVec3("material_diffuse_color", 0.8f, 0.1f, 0.1f); // No longer needed
            // ourShader.setVec3("material_specular_color", 0.5f, 0.5f, 0.5f); // No longer needed
            // glBindVertexArray(cubeVAO); // No longer needed
            for (const GameItem& item : gameItems)
            {
                if (item.isAlive)
                {
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, item.position);

                    // --- FIX: Translate down by radius ---
                    // The item.position is the *center* (y=0.5), but the model's origin
                    // is at its *base* (y=0). We shift it down so the base rests on the ground.
                    model = glm::translate(model, glm::vec3(0.0f, -ITEM_RADIUS, 0.0f));
                    // --- END FIX ---

                    // --- NEW: Apply rotation and scale for the model ---
                    // (Assuming it needs the same "stand up" rotation as the dog)
                    // model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // REMOVED: This model might already be Y-up
                    // (Models are often different sizes, 0.05f is a guess)
                    model = glm::scale(model, glm::vec3(0.3f)); // INCREASED: Made the model larger (was 0.05f)

                    ourShader.setMat4("model", model);
                    // glDrawArrays(GL_TRIANGLES, 0, 36); // Replaced
                    gummyModel.Draw(ourShader); // Draw the gummy model
                }
            }
            // glBindVertexArray(0); // No longer needed
            // --- END MODIFICATION ---
        }

        // --- Draw Obstacles ---
        ourShader.setInt("has_diffuse_texture", 0);
        ourShader.setVec3("material_diffuse_color", 0.4f, 0.4f, 0.5f); // Dark bluish-gray
        ourShader.setVec3("material_specular_color", 0.1f, 0.1f, 0.1f); // Not very shiny
        glBindVertexArray(cubeVAO);

        // --- 2a. Draw Static Obstacles ---
        for (const glm::vec3& obsPos : staticObstacles) // Renamed
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obsPos);
            // We can raise them slightly to be at the same level as items
            model = glm::translate(model, glm::vec3(0.0f, OBSTACLE_RADIUS - ITEM_RADIUS, 0.0f));
            model = glm::scale(model, glm::vec3(OBSTACLE_RADIUS * 2.0f));
            ourShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // --- 2b. Draw Moving Obstacles ---
        for (const MovingObstacle& obs : movingObstacles)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obs.position); // Use current position
            // We can raise them slightly to be at the same level as items
            model = glm::translate(model, glm::vec3(0.0f, OBSTACLE_RADIUS - ITEM_RADIUS, 0.0f));
            model = glm::scale(model, glm::vec3(OBSTACLE_RADIUS * 2.0f));
            ourShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        glBindVertexArray(0);
        // --- END NEW ---


        // Draw Car (unchanged)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, carPos);
        model = glm::rotate(model, glm::radians(carYaw + 180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // --- FIX: PITCH THE MODEL UP ---
        // Models often export "face-down" (Z-up instead of Y-up)
        // We rotate it -90 degrees on the X-axis to make it "stand up".
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        // --- FIX: Scale the model down ---
        // The original model is huge, so we scale it to 10% (0.1f) of its size.
        model = glm::scale(model, glm::vec3(0.1f)); // Was 1.0f
        ourShader.setMat4("model", model);
        carModel.Draw(ourShader);
        // --- END 3D RENDER ---


        // --- IMGUI 2D UI RENDER (REMOVED) ---


        // Swap buffers / poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);

    // --- IMGUI CLEANUP (REMOVED) ---

    glfwTerminate();
    return 0;
}

// ----- Input (driving controls) -----
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Only allow driving if in PLAYING state
    if (currentState == GameState::PLAYING)
    {
        bool forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        bool turnLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool turnRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool handbrake = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

        // accelerate / brake
        if (forward) {
            carForwardSpeed += CAR_ACCEL * deltaTime;
            if (carForwardSpeed > CAR_MAX_SPEED) carForwardSpeed = CAR_MAX_SPEED;
        }
        else if (backward) {
            if (carForwardSpeed > 0.1f) { carForwardSpeed -= CAR_BRAKE * deltaTime; }
            else {
                carForwardSpeed -= CAR_ACCEL * 0.5f * deltaTime;
                if (carForwardSpeed < CAR_REVERSE_MAX) carForwardSpeed = CAR_REVERSE_MAX;
            }
        }
        else { // friction
            if (carForwardSpeed > 0.0f) {
                carForwardSpeed -= CAR_FRICTION * deltaTime;
                if (carForwardSpeed < 0.0f) carForwardSpeed = 0.0f;
            }
            else {
                carForwardSpeed += CAR_FRICTION * deltaTime;
                if (carForwardSpeed > 0.0f) carForwardSpeed = 0.0f;
            }
        }

        // handbrake
        if (handbrake) {
            float brakeForce = (carForwardSpeed > 0.0f) ? -CAR_BRAKE : CAR_BRAKE;
            carForwardSpeed += brakeForce * 2.0f * deltaTime;
            // Check if speed crossed zero
            if (std::signbit(carForwardSpeed) != std::signbit(brakeForce) && carForwardSpeed != 0.0f) {
                carForwardSpeed = 0.0f;
            }
        }

        // steering
        float speedFactor = 1.0f;
        float absSpeed = fabs(carForwardSpeed);
        if (absSpeed > 0.1f) speedFactor = glm::clamp(1.0f - (absSpeed / CAR_MAX_SPEED) * 0.5f, 0.35f, 1.0f);

        if (absSpeed > 0.05f) {
            float turnDirection = (carForwardSpeed > 0.0f) ? 1.0f : -1.0f;
            if (turnLeft) {
                carYaw -= CAR_TURN_SPEED * speedFactor * deltaTime * turnDirection;
                if (carYaw <= -360.0f) carYaw += 360.0f;
            }
            if (turnRight) {
                carYaw += CAR_TURN_SPEED * speedFactor * deltaTime * turnDirection;
                if (carYaw >= 360.0f) carYaw -= 360.0f;
            }
        }
    }
}

// ----- Callbacks -----
void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* /*window*/, double xposIn, double yposIn)
{
    // --- IMGUI Check Removed ---

    // Only control camera if PLAYING
    if (currentState == GameState::PLAYING)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);
        if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
        float xoffset = xpos - lastX; float yoffset = lastY - ypos;
        lastX = xpos; lastY = ypos;
        // Mouse movement is ignored in this camera mode, but we leave the logic
        // in case you want to re-enable mouse-look later.
    }
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    // --- IMGUI Check Removed ---

    camera.ProcessMouseScroll(static_cast<float>(yoffset)); // Controls FOV
}


