#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > key1(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("key1.opus"));
});
Load< Sound::Sample > key2(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("key2.opus"));
});
Load< Sound::Sample > key3(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("key3.opus"));
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Hip.FL") hip = &transform;
		else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	}
	if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("GO LEFT", 1));
		choices.push_back(std::pair<std::string, int>("GO RIGHT", 2));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
					"YOU AND YOUR THREE COMPANIONS HAVE SHARPENED YOUR SWORDS AND TIGHTENED YOUR\n ARMOR. YOU STAND AT THE ENTRANCE OF THE CAVE HEADING INTO THE MOUNTAIN WHERE\n THE DRAGON LIES. "
"THE TUNNEL BEFORE YOU HAS TWO BRANCHES:\n RIGHT, HEADING UP TOWARD THE PEAK OF THE MOUNTAIN; AND LEFT, CURVING DOWN\n INTO ITS DEPTHS."
, choices));
	}

	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("TIE YOUR ROPE TO A BOULDER AND USE IT TO CLIMB DOWN THE PIT.", 3));
		choices.push_back(std::pair<std::string, int>("DOUBLE BACK AND TAKE THE OTHER PATH.", 2));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
					"ON THE LEFT PATH, YOU ALL BLUSTER AND JOKE FOR A FEW MINUTES\n, BUT SOON FALL INTO SILENCE, CONCIOUS OF THE STONE ACCUMULATING ABOVE YOU.\n AFTER CONTINUING FOR SOME TIME, SLOWLY BUT STEADILY MAKING YOUR WAY DOWN,\n THE TUNNEL DROPS AWAY TO REVEAL A DARK PIT ABOUT 15 FEET ACROSS.\n"

"AFTER SPENDING A FEW MINUTES DISCUSSING YOUR OPTIONS, THE PARTY DECIDES TO:"
, choices));
	}

	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("ASK THE PARTY", 4));
		choices.push_back(std::pair<std::string, int>("KEEP GOING", 9));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
					"YOU ALL DECIDE THAT A DRAGON WOULD PROBABLY WANT TO BE\n CLOSER TO THE TOP OF THE MOUNTAIN SO IT COULD FLY, AND\n BEGIN FOLLOWING THE PATH TO THE RIGHT. THE TUNNEL\n TWISTS AND TURNS, AND AT EACH INTERSECTION YOU CHOOSE\n THE PATH HEADING FURTHER UP.\n"

"DON'T YOU THINK YOU'VE SEEN THIS TUNNEL BEFORE?"
, choices));
	}
	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("CUT THE ROPE", 5));
		choices.push_back(std::pair<std::string, int>("THIS ISNT A GOOD IDEA", 4));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
					"HAVING DECIDED ON THE MORE ADVENTUROUS OPTION, YOU\n TIE THE ROPE TO THE BOULDER AND WATCH AS THE FIRST\n PERSON STARTS BACKING TOWARD THE PIT,\n FEEDING THE ROPE THROUGH THEIR HANDS.\n"

"YOU'RE STRUCK BY A SUDDEN, OVERWHELMING FEELING."
, choices));
	}
	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("YOU TRACE YOUR WAY BACK TO WHERE FAINT TRACES OF DAYLIGHT STILL SEEP INTO THE DARK TUNNELS", 0));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
					"YOU STOP MOVING; THE OTHERS NOTICE AND TURN.\n"
"DON'T YOU THINK, YOU SAY SLOWLY, THAT WE'VE TAKEN A WRONG TURN SOMEWHERE?\n"
"TO YOUR SURPRISE, THE AGREE WITHOUT HESITATION.\n"
, choices));
	}
	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("YOU CUT THE ROPE", 6));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
					"AS THE OTHER TWO LEAN OVER THE EDGE, CALLING ENCOURAGEMENT\n TO THE CLIMBER, YOU SLOWLY DRAW YOUR SWORD FROM ITS SHEATH\n AND REST THE BLADE LIGHTLY AGAINST THE TAUT ROPE. DO YOU\n EVEN KNOW THEM? DO THEY MATTER TO YOU AT ALL, HERE\n UNDERNEATH THE DIRT AND STONE?"
, choices));
	}
	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("GAME OVER", 8));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
					"THEY GASP AS THEY FALL, BUT DON'T SCREAM. THE OTHERS\n STARE INTO THE DARK PIT, FROZEN, UNTIL THE IMPACT ECHOES\n FROM FAR BELOW.\n"

"THEY TURN TO YOU AND DRAW THEIR SWORDS."
, choices));
	}
	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("GAME OVER", 8));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
					"YOU CARRY ON, THE ONLY SOUND THE CLINK OF YOUR EQUIPMENT.\n THE TUNNELS WIND UP, THEN DOWN, PETERING INTO TIGHT\n PASSAGES THAT WIDEN JUST WHEN YOU'RE\n SURE YOU CAN GO NO FURTHER.\n"

, choices));
	}
	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("RESTART", 0));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
"THE DRAGON YAWNS, AND SETTLES ITSELF MORE COMFORTABLY.\n IT'S A GOOD DAY WHEN ONE'S HOARD GROWS BY FOUR.\n"
, choices));
	}
	{
		std::vector<std::pair<std::string, int>> choices;
		choices.push_back(std::pair<std::string, int>("YOU CONTINUE ON", 7));
		choices.push_back(std::pair<std::string, int>("YOU SECOND GUESS YOURSELF", 4));

		decisions.push_back(std::pair<std::string, std::vector<std::pair<std::string, int>>>(
"NO, YOU MUST BE MISTAKEN. THIS MOUNTAIN CAN'T BE BIG ENOUGH\n FOR A MAZE OF THIS SIZE; YOU MUST STILL BE ON THE ONLY VIABLE PATH."
, choices));
	}
	
	
	currentOptions = decisions[0].second;
	currentMessage = decisions[0].first;
	currentMessageIdx = 0;
	//start music loop playing:
	//
	// (note: position will be over-ridden in update())
	/* leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f); */
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_x) {
			right.downs += 1;
			right.pressed = true;
			if(currentOptions.size() > 2){
				currentMessage = decisions[currentOptions[2].second].first;
				currentOptions = decisions[currentOptions[2].second].second;
				currentMessageIdx = 0;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			if(currentOptions.size() > 0){
				currentMessage = decisions[currentOptions[0].second].first;
				currentOptions = decisions[currentOptions[0].second].second;
				currentMessageIdx = 0;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			if(currentOptions.size() >1){
				currentMessage = decisions[currentOptions[1].second].first;
				currentOptions = decisions[currentOptions[1].second].second;
				currentMessageIdx = 0;
			}
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

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	hip->rotation = hip_base_rotation * glm::angleAxis(
		glm::radians(5.0f * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	upper_leg->rotation = upper_leg_base_rotation * glm::angleAxis(
		glm::radians(7.0f * std::sin(wobble * 2.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);
	lower_leg->rotation = lower_leg_base_rotation * glm::angleAxis(
		glm::radians(10.0f * std::sin(wobble * 3.0f * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	//move sound to follow leg tip position:
	/* leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f); */

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
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	if(currentMessageIdx < currentMessage.length()){
		typeTimer += elapsed;
		if(typeTimer >= 0.01f){
			typeTimer = 0;
			currentMessageIdx++;
			float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			if(r < 0.3f){
				Sound::play(*key1, 1.0f);
			}else if(r < 0.6f){
				Sound::play(*key2, 1.0f);
			}else{
				Sound::play(*key3, 1.0f);
			}
		}
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.
	

	glDisable(GL_DEPTH_TEST);
	CustomText::draw_text(currentMessage.substr(0, currentMessageIdx + 1).c_str(), glm::vec2(100, 600), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	int currentHeight = 300;

	if(currentMessageIdx == currentMessage.length()){
		for(auto it = currentOptions.begin(); it < currentOptions.end(); it++){
			CustomText::draw_text(it->first.c_str(), glm::vec2(100, currentHeight), 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
			currentHeight -= 100;
		}
	}
	GL_ERRORS();
}

glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_local_to_world() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
