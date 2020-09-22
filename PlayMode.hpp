#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

  struct Target {
    Scene::Transform* self = nullptr;

    float radius;
    float pos_speed;
    glm::vec3 pos_axis;
    glm::vec3 initial_pos;
    float cur_pos;

    float self_rot_speed;
    glm::vec3 self_rot_axis;
    float cur_rot;
  } cake, eggb, eggb2, pan, sand;

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

  virtual void setup();
  virtual void reset_target(Target* target);

  // Game constants
  static constexpr int shots_max = 10;
  static constexpr float time_max = 10.0f;
  static constexpr float target_size = 30.0f;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

  // Game stats
  int shots_left;
  int score;
  float time;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

  std::vector<Target*> targets;

	//camera:
	Scene::Camera *camera = nullptr;

  bool running;

};
