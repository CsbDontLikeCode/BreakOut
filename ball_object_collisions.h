#ifndef BALLOBJECT_H
#define BALLOBJECT_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "game_object.h"
#include "texture.h"
#include "power_up.h"

class BallObject : public GameObject
{
public:
	float   Radius;
	bool    Stuck;
	GLboolean Sticky = GL_FALSE, PassThrough = GL_FALSE;

	BallObject();
	BallObject(glm::vec2 pos, float radius, glm::vec2 velocity, Texture2D sprite);

	glm::vec2 Move(float dt, unsigned int window_width);

	void      Reset(glm::vec2 position, glm::vec2 velocity);
};

#endif