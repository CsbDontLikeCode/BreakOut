#include <algorithm>

#include "game.h"
#include "resource_manager.h"
#include "sprite_renderer.h"
#include "game_object.h"
#include "ball_object_collisions.h"
#include "particle_generator.h"
#include "post_processor.h"
#include "power_up.h"

#include <iostream>

SpriteRenderer* Renderer;

GameObject* Player;

BallObject* Ball;

ParticleGenerator* Particles;

PostProcessor* Effects;

float ShakeTime = 0.0f;

Game::Game(unsigned int width, unsigned int height)
	:State(GAME_ACTIVE), Keys(),Width(width), Height(height)
{

}

Game::~Game() 
{
	delete Renderer;
    delete Player;
    delete Ball;
    delete Particles;
    delete Effects;
}

void Game::Init()
{   
    ResourceManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", nullptr, "sprite");
    ResourceManager::LoadShader("shaders/particle.vs", "shaders/particle.frag", nullptr, "particle");
    ResourceManager::LoadShader("shaders/final.vs", "shaders/final.frag", nullptr, "postprocessing");

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width), static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("sprite").SetVector3f("spriteColor", glm::vec3(0.0f, 1.0f, 1.0f));
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);
    Shader shader = ResourceManager::GetShader("sprite");
    Renderer = new SpriteRenderer(shader);
    Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);

    ResourceManager::LoadTexture("resources/textures/awesomeface.png", true, "face");
    ResourceManager::LoadTexture("resources/textures/background.jpg", false, "background");
    ResourceManager::LoadTexture("resources/textures/block.png", false, "block");
    ResourceManager::LoadTexture("resources/textures/block_solid.png", false, "block_solid");
    ResourceManager::LoadTexture("resources/textures/paddle.png", true, "paddle");
    ResourceManager::LoadTexture("resources/textures/particle.png", true, "particle");
    ResourceManager::LoadTexture("resources/textures/powerup_chaos.png", true, "tex_chaos");
    ResourceManager::LoadTexture("resources/textures/powerup_confuse.png", true, "tex_confuse");
    ResourceManager::LoadTexture("resources/textures/powerup_increase.png", true, "tex_increase");
    ResourceManager::LoadTexture("resources/textures/powerup_passthrough.png", true, "tex_passthrough");
    ResourceManager::LoadTexture("resources/textures/powerup_speed.png", true, "tex_speed");
    ResourceManager::LoadTexture("resources/textures/powerup_sticky.png", true, "tex_sticky");

    GameLevel one;
    one.Load("resources/levels/one.lvl", this->Width, this->Height / 2);
    GameLevel two;
    two.Load("resources/levels/two.lvl", this->Width, this->Height / 2);
    GameLevel three;
    three.Load("resources/levels/three.lvl", this->Width, this->Height / 2);
    GameLevel four;
    four.Load("resources/levels/four.lvl", this->Width, this->Height / 2);

    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Level = 0;

    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));

    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -BALL_RADIUS * 2.0f);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("face"));

    Particles = new ParticleGenerator(
        ResourceManager::GetShader("particle"),
        ResourceManager::GetTexture("particle"),
        500
    );
}

void Game::Update(float dt)
{
    Ball->Move(dt, this->Width);
    this->DoCollisions();
    this->UpdatePowerUps(dt);
    Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2.0f));

    if (ShakeTime > 0.0f)
    {
        ShakeTime -= dt;
        if (ShakeTime <= 0.0f)
            Effects->Shake = false;
    }

    if (Ball->Position.y >= this->Height)
    {
        this->ResetLevel();
        this->ResetPlayer();
    }
}

void Game::ProcessInput(float dt)
{
    if (this->State == GAME_ACTIVE)
    {
        float velocity = PLAYER_VELOCITY * dt;
        if (this->Keys[GLFW_KEY_A])
        {
            if (Player->Position.x >= 0.0f)
            {
                Player->Position.x -= velocity;
                if (Ball->Stuck)
                    Ball->Position.x -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_D])
        {
            if (Player->Position.x <= this->Width - Player->Size.x) 
            {
                Player->Position.x += velocity;
                if (Ball->Stuck)
                    Ball->Position.x += velocity;
            }
        }
        if (this->Keys[GLFW_KEY_SPACE])
            Ball->Stuck = false;
    }

}

void Game::Render()
{
    Texture2D texture = ResourceManager::GetTexture("face");
    Renderer->DrawSprite(texture, glm::vec2(200.0f, 200.0f), glm::vec2(300.0f, 400.0f), 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));

    if (this->State == GAME_ACTIVE)
    {
        Effects->BeginRender();

        Texture2D texture = ResourceManager::GetTexture("background");
        Renderer->DrawSprite(texture, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        this->Levels[this->Level].Draw(*Renderer);

        Player->Draw(*Renderer);

        for (PowerUp& powerUp : this->PowerUps)
            if (!powerUp.Destroyed)
                powerUp.Draw(*Renderer);

        Particles->Draw();

        Ball->Draw(*Renderer);

        Effects->EndRender();

        Effects->Render(glfwGetTime());
    }
}

void Game::ResetLevel()
{
    if (this->Level == 0)
        this->Levels[0].Load("resources/levels/one.lvl", this->Width, this->Height / 2);
    else if (this->Level == 1)
        this->Levels[1].Load("resources/levels/two.lvl", this->Width, this->Height / 2);
    else if (this->Level == 2)
        this->Levels[2].Load("resources/levels/three.lvl", this->Width, this->Height / 2);
    else if (this->Level == 3)
        this->Levels[3].Load("resources/levels/four.lvl", this->Width, this->Height / 2);
}

void Game::ResetPlayer()
{
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);
}

bool CheckCollision(GameObject& one, GameObject& two);
Collision CheckCollision(BallObject& one, GameObject& two);
Direction VectorDirection(glm::vec2 closest);
bool ShouldSpawn(unsigned int chance);
void ActivatePowerUp(PowerUp& powerUp);

void Game::DoCollisions()
{
    // 球与砖块碰撞检测
    for (GameObject& box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
            Collision collision = CheckCollision(*Ball, box);
            if (std::get<0>(collision))
            {
                if (!box.IsSolid)
                {
                    box.Destroyed = true;
                    this->SpawnPowerUps(box);
                }
                else
                {
                    ShakeTime = 0.05f;
                    Effects->Shake = true;
                }
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                if (!(Ball->PassThrough && !box.IsSolid)) {
                    if (dir == LEFT || dir == RIGHT) // Horizontal collision
                    {
                        Ball->Velocity.x = -Ball->Velocity.x; // Reverse horizontal velocity
                        // Relocate
                        GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
                        if (dir == LEFT)
                            Ball->Position.x += penetration; // Move ball to right
                        else
                            Ball->Position.x -= penetration; // Move ball to left;
                    }
                    else // Vertical collision
                    {
                        Ball->Velocity.y = -Ball->Velocity.y; // Reverse vertical velocity
                        // Relocate
                        GLfloat penetration = Ball->Radius - std::abs(diff_vector.y);
                        if (dir == UP)
                            Ball->Position.y -= penetration; // Move ball bback up
                        else
                            Ball->Position.y += penetration; // Move ball back down
                    }
                }
            }
        }
    }

    // 球与玩家碰撞检测
    Collision result = CheckCollision(*Ball, *Player);
    if (!Ball->Stuck && std::get<0>(result))
    {
        float centerBoard = Player->Position.x + Player->Size.x / 2.0f;
        float distance = (Ball->Position.x + Ball->Radius) - centerBoard;
        float percentage = distance / (Player->Size.x / 2.0f);
        float strength = 2.0f;
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity);
        Ball->Velocity.y = -1.0f * abs(Ball->Velocity.y);
        Ball->Stuck = Ball->Sticky;
        int i = 0;
    }

    // 道具与玩家碰撞检测
    for (PowerUp& powerUp : this->PowerUps)
    {
        if (!powerUp.Destroyed)
        {
            if (powerUp.Position.y >= this->Height)
                powerUp.Destroyed = GL_TRUE;
            if (CheckCollision(*Player, powerUp))
            {
                ActivatePowerUp(powerUp);
                powerUp.Destroyed = GL_TRUE;
                powerUp.Activated = GL_TRUE;
            }
        }
    }
}

bool CheckCollision(GameObject& one, GameObject& two)
{
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
        two.Position.x + two.Size.x >= one.Position.x;
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
        two.Position.y + two.Size.y >= one.Position.y;
    return collisionX && collisionY;
}

Collision CheckCollision(BallObject& one, GameObject& two)
{
    glm::vec2 center(one.Position + one.Radius);
    glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
    glm::vec2 aabb_center(
        two.Position.x + aabb_half_extents.x,
        two.Position.y + aabb_half_extents.y
    );
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    glm::vec2 closest = aabb_center + clamped;
    difference = closest - center;
    if (glm::length(difference) < one.Radius)
        return std::make_tuple(true, VectorDirection(difference), difference);
    else
        return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}

Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),	// up
        glm::vec2(1.0f, 0.0f),	// right
        glm::vec2(0.0f, -1.0f),	// down
        glm::vec2(-1.0f, 0.0f)	// left
    };
    float max = 0.0f;
    unsigned int best_match = -1;
    for (unsigned int i = 0; i < 4; i++)
    {
        float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}

bool ShouldSpawn(unsigned int chance)
{
    unsigned int random = rand() % chance;
    return random == 0;
}

void Game::SpawnPowerUps(GameObject& block)
{
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("tex_speed")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("sticky", glm::vec3(0.5f, 0.5f, 1.0f), 10.0f, block.Position, ResourceManager::GetTexture("tex_sticky")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("tex_passthrough")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 3.0f, block.Position, ResourceManager::GetTexture("tex_increase")));
    if (ShouldSpawn(25)) // 负面道具被更频繁地生成
        this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 3.0f, block.Position, ResourceManager::GetTexture("tex_confuse")));
    if (ShouldSpawn(25))
        this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 3.0f, block.Position, ResourceManager::GetTexture("tex_chaos")));
}

void ActivatePowerUp(PowerUp& powerUp)
{
    // 根据道具类型发动道具
    if (powerUp.Type == "speed")
    {
        Ball->Velocity *= 1.2;
    }
    else if (powerUp.Type == "sticky")
    {
        Ball->Sticky = GL_TRUE;
        Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
    }
    else if (powerUp.Type == "pass-through")
    {
        Ball->PassThrough = GL_TRUE;
        Ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
    }
    else if (powerUp.Type == "pad-size-increase")
    {
        Player->Size.x += 50;
    }
    else if (powerUp.Type == "confuse")
    {
        if (!Effects->Chaos)
            Effects->Confuse = GL_TRUE; // 只在chaos未激活时生效，chaos同理
    }
    else if (powerUp.Type == "chaos")
    {
        if (!Effects->Confuse)
            Effects->Chaos = GL_TRUE;
    }
}

// 判断是否有其他同类型道具处于激活状态
GLboolean IsOtherPowerUpActive(std::vector<PowerUp>& powerUps, std::string type)
{
    for (const PowerUp& powerUp : powerUps)
    {
        if (powerUp.Type == type && powerUp.Activated == true) {
        //if (powerUp.Type == type) {
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

// 更新道具状态
void Game::UpdatePowerUps(GLfloat dt) {
    for (PowerUp& powerUp : this->PowerUps) 
    {
        // 更新道具位置
        powerUp.Position += powerUp.Velocity * dt;
        // 判断道具是否处于激活状态
        if (powerUp.Activated) {
            // 道具属性Duration(持续时间)处理：减去时间增量
            powerUp.Duration -= dt;
            // 判断道具持续时间是否已经耗尽
            if (powerUp.Duration <= 0.0f) 
            {
                // 销毁道具
                powerUp.Activated = GL_FALSE;
                // 停用效果
                if (powerUp.Type == "sticky") //粘连
                {
                    // 判断无同类型道具处于激活状态,下同
                    if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
                    {
                        Ball->Sticky = GL_FALSE;
                        Player->Color = glm::vec3(1.0f);
                    }
                }
                else if(powerUp.Type == "pass-through") //横冲
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
                    {
                        Ball->PassThrough = GL_FALSE;
                        Player->Color = glm::vec3(1.0f);
                        Ball->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "confuse") // 混色
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
                    {
                        Effects->Confuse = GL_FALSE;
                    }
                }
                else if (powerUp.Type == "chaos")   //混乱
                {
                    if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
                    {
                        Effects->Chaos = GL_FALSE;
                    }
                }
            }
        }
        // 删除处于销毁状态且未激活的所有道具
        this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
            [](const PowerUp& powerUp) { return powerUp.Destroyed && !powerUp.Activated; }), this->PowerUps.end());
    }
}