#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#define _USE_MATH_DEFINES

#include <cmath>
#include <random>
#include <sstream>
#include <string>

GLuint food_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > food_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("brunch_food.pnct"));
	food_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > food_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("brunch_food.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = food_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = food_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*food_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
    std::cout << transform.name << std::endl;
    if (transform.name == "cakePlate") {
      cake.self = &transform;
      targets.push_back(&cake);
    } else if (transform.name == "eggBaconPlate") {
      eggb.self = &transform;
      targets.push_back(&eggb);
    } else if (transform.name == "eggBaconPlate2") {
      eggb2.self = &transform;
      targets.push_back(&eggb2);
    } else if (transform.name == "pancakePlate") {
      pan.self = &transform;
      targets.push_back(&pan);
    } else if (transform.name == "sandwichPlate") {
      sand.self = &transform;
      targets.push_back(&sand);
    }
  }

	if (cake.self == nullptr) throw std::runtime_error("Cake not found.");
	if (eggb.self == nullptr) throw std::runtime_error("Egg bacon 1 not found.");
	if (eggb2.self == nullptr) throw std::runtime_error("Egg bacon 2 not found.");
	if (pan.self == nullptr) throw std::runtime_error("Pancake not found.");
	if (sand.self == nullptr) throw std::runtime_error("Sandwich not found.");

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

  setup();
}

PlayMode::~PlayMode() {
}

void PlayMode::setup() {
  shots_left = shots_max;
  score = 0;
  time = time_max;

  for(auto& t : targets) {
    reset_target(t);
  }

  running = true;

  camera->transform->position.x = 30;
  camera->transform->position.y = 36;
  camera->transform->position.z = 305;
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
    if(!running) {
      // Restart the game
      setup();
      return true;
    }

		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
      // Shoot targets
      shots_left--;

      for(auto& t : targets) {
        float dist = glm::length(glm::cross(t->self->position - glm::vec3(0.0f, 0.0f, 0.0f), t->self->position - camera->transform->position)) / glm::length(camera->transform->position); 

        if(dist < target_size) {
          // Hit
          score++;
          reset_target(t);
        }
      }
      return true;
    } else if (evt.key.keysym.sym == SDLK_r) {
      // Quick restart
      setup();
      return true;
    }
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::reset_target(Target* target) {
  static std::mt19937 mt;

  target->self_rot_speed = (mt() / float(mt.max())) * 1 + 1;

  float rot_axis_x = (mt() / float(mt.max())) * 2 - 1;
  float rot_axis_angle = (mt() / float(mt.max())) * 2 * float(M_PI);
  target->self_rot_axis.x = rot_axis_x;
  target->self_rot_axis.y = (1 - rot_axis_x * rot_axis_x) * std::sin(rot_axis_angle);
  target->self_rot_axis.z = (1 - rot_axis_x * rot_axis_x) * std::cos(rot_axis_angle);

  target->cur_rot = 0;

  target->radius = (mt() / float(mt.max())) * 50 + 50;
  target->pos_speed = (mt() / float(mt.max())) * 0.25 + 0.25;

  float pos_axis_x = (mt() / float(mt.max())) * 2 - 1;
  float pos_axis_angle = (mt() / float(mt.max())) * 2 * float(M_PI);
  target->pos_axis.x = pos_axis_x;
  target->pos_axis.y = (1 - pos_axis_x * pos_axis_x) * std::sin(pos_axis_angle);
  target->pos_axis.z = (1 - pos_axis_x * pos_axis_x) * std::cos(pos_axis_angle);

  target->initial_pos = target->radius * glm::normalize(
      glm::cross(target->pos_axis, target->self_rot_axis));

  target->cur_pos = 0;
}

void PlayMode::update(float elapsed) {
  if(!running) return;

  time -= elapsed;
  if(shots_left <= 0 || time < 0) {
    running = false;
  }

  for(auto& t : targets) {
    t->cur_rot += elapsed;
    t->cur_pos += elapsed;

    // Update self-rotation
    //t->self->rotation *= glm::angleAxis(glm::radians(t->self_rot * std::sin(t->cur_rot * float(M_PI))), t->self_rot_axis);
    t->self->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f) * glm::angleAxis(
    t->cur_rot * t->self_rot_speed * float(M_PI),
		t->self_rot_axis);

    // Update position
    // Calculate rotation with Rodrigues' rotation formula
    float angle = t->cur_pos * t->pos_speed * float(M_PI);
    glm::vec3 axis = t->pos_axis;
    glm::vec3 initial = t->initial_pos;
    t->self->position = std::cos(angle) * initial +
      std::sin(angle) * glm::cross(axis, initial) +
      (1 - std::cos(angle)) * (axis * initial) * axis;
  }

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 forward = -frame[2];

		camera->transform->position += move.x * right + move.y * forward;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	GL_ERRORS();
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	GL_ERRORS();
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

    std::ostringstream text;
    text << "Score " << score << "  Shots left " << shots_left << "  Time left " << time;
		constexpr float H = 0.09f;
		lines.draw_text(text.str(),
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(text.str(),
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
