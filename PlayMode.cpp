#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "TexProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include "load_save_png.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

std::vector<std::string> dialogs;
std::vector<std::string> keys_acquired;
std::vector<GLuint> char_vaos;
float dialog_time = 0.0f;

GLuint image_vao = 0;
GLuint font_tex = 0;
GLuint scenes[20];

struct Door;
struct Interactable;

struct Area {
    int texture;
    std::vector<std::string> intro_message;
    std::vector<std::string> seen_message;
    std::vector<Door*> doors;
    std::vector<Interactable*> keys;
    mutable bool seen = false;

    Area(int id, std::vector<std::string> msg_intro, std::vector<std::string> msg_seen) {
        texture = id;
        intro_message = msg_intro;
        seen_message = msg_seen;
    }

    void visit() {
        if (!seen) {
            dialogs.insert(dialogs.end(), intro_message.begin(), intro_message.end());
        } else
            dialogs.insert(dialogs.end(), seen_message.begin(), seen_message.end());
    }
};

Area* current_area;

struct Door {
    std::string required_key = "";
    std::vector<std::string> fail_message;
    Area* destination;
    glm::vec2 position;
    glm::vec2 size;

    Door(glm::vec2 l, glm::vec2 s, Area* dest, std::string key, std::vector<std::string> fail) {
        position = l;
        size = s;
        destination = dest;
        required_key = key;
        fail_message = fail;
    }

    void interact() {
        if (required_key != "") {
            bool found = false;
            for (std::string s: keys_acquired) {
                if (s == required_key)
                    found = true;
            }

            if (!found)
            {
                dialogs.insert(dialogs.end(), fail_message.begin(), fail_message.end());
                return;
            }
        }

        current_area = destination;
        dialogs.insert(dialogs.end(), destination->intro_message.begin(), destination->intro_message.end());
    }
};

struct Interactable {
    glm::vec2 position;
    glm::vec2 size;
    std::vector<std::string> keys_within;
    mutable std::vector<std::string> discover_message;
    mutable std::vector<std::string> seen_message;
    mutable bool seen = false;

    Interactable(glm::vec2 l, glm::vec2 s, std::vector<std::string> keys, std::vector<std::string> disc_msg, std::vector<std::string> seen_msg) {
        position = l;
        size = s;
        keys_within = keys;

        discover_message = disc_msg;
        seen_message = seen_msg;
    }

    void interact() {
        if (!seen) {
            dialogs.insert(dialogs.end(), discover_message.begin(), discover_message.end());
            keys_acquired.insert(keys_acquired.end(), keys_within.begin(), keys_within.end());
        } else {
            dialogs.insert(dialogs.end(), seen_message.begin(), seen_message.end());
        }

        seen = true;
    }
};

GLuint gen_texture(std::string tex) {
    GLuint i = 0;
    glGenTextures(1, &i);
    glm::uvec2 size;
    std::vector<glm::u8vec4> data;
    load_png(data_path(tex), &size, &data, LowerLeftOrigin);

    glBindTexture(GL_TEXTURE_2D, i);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    GL_ERRORS();

    return i;
}

// Inspired by Jim McCann's message in the course Discord on how to create a textured quad:
// https://discord.com/channels/1144815260629479486/1154543452520984686/1156347836888256582

// Generates a model and vao for a texture
GLuint gen_image(glm::vec2 pos, glm::vec2 size, float u1, float v1, float u2, float v2) {
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    Load<TexProgram> &program = tex_program;
    glUseProgram(program->program);

    GLuint coord_buf = 0;
    glGenBuffers(1, &coord_buf);
    std::vector<glm::vec3> positions;

    positions.emplace_back(glm::vec3(pos.x, pos.y, 0.0f));
    positions.emplace_back(glm::vec3(pos.x + size.x, pos.y, 0.0f));
    positions.emplace_back(glm::vec3(pos.x + size.x, pos.y + size.y, 0.0f));
    positions.emplace_back(glm::vec3(pos.x, pos.y + size.y, 0.0f));

    glBindBuffer(GL_ARRAY_BUFFER, coord_buf);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * 3 * 4, positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(program->Position_vec4, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(program->Position_vec4);

    GLuint tex_buf = 0;
    glGenBuffers(1, &tex_buf);
    std::vector<glm::vec2> tex_coords;

    tex_coords.emplace_back(glm::vec2(u1, v1));
    tex_coords.emplace_back(glm::vec2(u2, v1));
    tex_coords.emplace_back(glm::vec2(u2, v2));
    tex_coords.emplace_back(glm::vec2(u1, v2));

    glBindBuffer(GL_ARRAY_BUFFER, tex_buf);
    glBufferData(GL_ARRAY_BUFFER, tex_coords.size() * 2 * 4, tex_coords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(program->TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(program->TexCoord_vec2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    GL_ERRORS();

    return vao;
}

void draw_image(GLuint image, GLuint tex, glm::vec4 color, float x, float y, float x_scale, float y_scale) {
    glUseProgram(tex_program->program);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(image);

    glUniformMatrix4fv(tex_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(glm::mat4(x_scale, 0.0f, 0.0f, 0.0f,  0.0f, y_scale, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, 0.0f,  x, y, 0.0f, 1.0f)));
    glUniform4f(tex_program->COLOR_vec4, color.x, color.y, color.z, color.w);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    GL_ERRORS();

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
    GL_ERRORS();
}

float char_widths[] = {32.5156f, 26.0312f, 38.2344f, 72.9531f, 53.125f, 86.625f, 65.8281f,21.2344f,42.7188f, 42.7188f, 54.25f, 53.9062f, 19.2031f, 57.125f,20.5156f, 63.9219f,
                       91.7031f, 35.4531f, 75.25f, 71.9219f, 77.3438f,75.0469f, 73.2031f, 62.75f, 74.125f, 74.4219f, 24.3125f, 25.1406f,52.5469f, 59.125f, 52.5469f, 66.5469f,
                       66.1094f, 83.4531f, 80.9062f,74.7031f, 82.3281f, 75.5312f, 73.8281f, 82.3281f, 84.9062f, 27.6406f,73.4375f, 81.3438f, 73.6406f, 96.2344f, 84.7188f, 86.0312f,
                       80.6094f,86.0312f, 82.7188f, 77.0938f, 72.6094f, 83.3438f, 77.25f, 100.25f,78.7188f, 78.125f, 71.3438f, 36.5312f, 65.9219f, 36.5312f, 43.0156f,68.1094f,
                       55.7188f, 65.3281f, 69.0469f, 61.2344f, 70.0156f, 65.3281f,54.7812f, 69.0469f, 70.3125f, 25.0469f, 25.0469f, 69.1406f, 31.3438f,87.3594f, 69.625f, 68.2188f,
                       69.0469f, 69.0469f, 54.5469f, 59.0312f,56.3438f, 69.625f, 67.5312f, 91.3125f, 69.1406f, 70.3125f, 60.1094f,42.4375f, 26.0312f, 42.4375f, 63.3281f};

void draw_string(std::string text, glm::vec4 color, float x, float y, float x_scale, float y_scale) {
    float advance = 0.0f;
    int count = 0;
    for (char c: text) {
       if (count > dialog_time)
           return;

       draw_image(char_vaos[c - 32], font_tex, color, x + advance / 48.0f * x_scale, y, x_scale, y_scale);
       advance += char_widths[c - 32];
       count++;
    }
}

void gen_chars()
{
    for (int i = 0; i < 95; i++)
    {
        GLuint vao = gen_image(glm::vec2(-1.0f, -1.0f), glm::vec2(2.0f, 2.0f), (i % 32) / 32.0f, 1.0f - (i / 32 + 1) / 4.0f, ((i % 32) + 1) / 32.0f, 1.0f - (i / 32) / 4.0f);
        char_vaos.emplace_back(vao);
    }
}

Load< Scene > empty_scene(LoadTagDefault, []() -> Scene const * {
    Scene* s = new Scene(data_path("data/emptyscene.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){});
    return s;
});

GLuint dialog_tex;
PlayMode::PlayMode() : scene(*empty_scene) {
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

    gen_chars();
    image_vao = gen_image(glm::vec2(-1.0f, -1.0f), glm::vec2(2.0f, 2.0f), 0.0f, 0.0f, 1.0f, 1.0f);
    font_tex = gen_texture("data/font.png");
    dialog_tex = gen_texture("data/dialog.png");

    for (int i = 0; i < 8; i++)
    {
        std::string s = "data/scene";
        s += std::to_string(i + 1);
        s += ".png";
        scenes[i] = gen_texture(s);
    }

    Area* a0 = new Area(0, std::vector<std::string>(), std::vector<std::string>());
    current_area = a0;

    Area* a1 = new Area(1, std::vector<std::string>(), std::vector<std::string>());
    Area* a2 = new Area(2, std::vector<std::string>(), std::vector<std::string>());
    Area* a3 = new Area(3, std::vector<std::string>(), std::vector<std::string>());
    Area* a4 = new Area(4, std::vector<std::string>(), std::vector<std::string>());
    Area* a5 = new Area(5, std::vector<std::string>(), std::vector<std::string>());
    Area* a6 = new Area(6, std::vector<std::string>(), std::vector<std::string>());
    Area* a7 = new Area(7, std::vector<std::string>(), std::vector<std::string>());

    a0->doors.emplace_back(new Door(glm::vec2(0.63f, -0.23f), glm::vec2(0.3f, 0.3f), a1, "", std::vector<std::string>()));
    a0->doors.emplace_back(new Door(glm::vec2(-0.1f, -0.8f), glm::vec2(0.3f, 0.3f), a3, "gravity-left", {"Hmm... this wall looks quite unclimable..."}));

    a1->doors.emplace_back(new Door(glm::vec2(-0.8f, -0.0f), glm::vec2(0.3f, 0.3f), a0, "", {}));
    a1->doors.emplace_back(new Door(glm::vec2(0.83f, -0.47f), glm::vec2(0.3f, 0.3f), a2, "", {}));
    a1->doors.emplace_back(new Door(glm::vec2(0.09f, 0.58f), glm::vec2(0.3f, 0.3f), a1, "no", {"Yikes, that antimatter looks dangerous, better stay away!"}));

    a2->doors.emplace_back(new Door(glm::vec2(-0.78f, 0.69f), glm::vec2(0.3f, 0.3f), a1, "", {}));
    a2->doors.emplace_back(new Door(glm::vec2(0.49f, 0.19f), glm::vec2(0.3f, 0.3f), a0, "gravity-left", {"This looks like a dangerous drop..."}));
    a2->keys.emplace_back(new Interactable(glm::vec2(0.30f, -0.67f), glm::vec2(0.3f, 0.3f), {"gravity-left"}, {"Auriga can now use left gravity!", "Perhaps this might give him a new perspective on drops and walls..."}, {"Auriga can now use gravity to climb walls!"}));

    a3->keys.emplace_back(new Interactable(glm::vec2(0.0f, -0.78f), glm::vec2(0.3f, 0.3f), {"blastoff"}, {"Auriga finds a jetpack!", "He feels the call of the third dimension...", "Click on Auriga to go for a flight!"}, {"Click on Auriga to go for a flight!"}));
    a3->doors.emplace_back(new Door(glm::vec2(0.11f, 0.56f), glm::vec2(0.3f, 0.3f), a0, "", {}));
    a3->doors.emplace_back(new Door(glm::vec2(0.0f, 0.0f), glm::vec2(0.3f, 0.3f), a4, "blastoff", {"Auriga feels the call of the 3rd dimension...", "But he has no way to take off!"}));

    a4->intro_message.emplace_back("Auriga soars through space, and eventually arrives at...");
    a4->doors.emplace_back(new Door(glm::vec2(0.0f, 0.0f), glm::vec2(2.0f, 2.0f), a5, "", {}));

    a5->intro_message.emplace_back("A mysterious floating square of grass...");
    a5->doors.emplace_back(new Door(glm::vec2(0.85f, 0.11f), glm::vec2(0.3f, 0.3f), a6, "", {}));
    a5->doors.emplace_back(new Door(glm::vec2(0.0f, -0.75f), glm::vec2(2.0f, 0.3f), a1, "no", {"Yikes, that antimatter looks dangerous, better stay away!"}));
    a5->doors.emplace_back(new Door(glm::vec2(0.0f, 0.75f), glm::vec2(2.0f, 0.3f), a1, "no", {"Yikes, that antimatter looks dangerous, better stay away!"}));
    a5->doors.emplace_back(new Door(glm::vec2(-0.75f, 0.0f), glm::vec2(0.3f, 2.0f), a1, "no", {"Yikes, that antimatter looks dangerous, better stay away!"}));

    a6->intro_message.emplace_back("Auriga suddenly gets teleported to a dark expanse of cosmos...");
    a6->intro_message.emplace_back("He only has a limited amount of fuel left!");
    a6->intro_message.emplace_back("Perhaps he can use echolocation to find his way...");
    a6->doors.emplace_back(new Door(glm::vec2(0.5f, 0.5f), glm::vec2(0.3f, 0.3f), a7, "", {}));
    a6->doors.emplace_back(new Door(glm::vec2(0.0f, 0.0f), glm::vec2(2.0f, 2.0f), a6, "no", {"Oh no, it sounds like there's a dangerous asteroid that way!"}));

    a7->intro_message.emplace_back("Yay! Auriga found his home moon!");
    a7->intro_message.emplace_back("He is overjoyed to be back home with his family!");

    dialogs.emplace_back("Auriga the astronaut is lost in space :(");
    dialogs.emplace_back("Perhaps you can help him out! Click around to check things out!");
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_MOUSEBUTTONDOWN){
        if (dialogs.size() > 0){
            if (dialog_time < dialogs.begin()->length())
                dialog_time = (float) (dialogs.begin()->length());
            else {
                dialogs.erase(dialogs.begin());
                dialog_time = 0;
            }
        } else {
            int mx;
            int my;

            SDL_GetMouseState(&mx, &my);

            float x = 2.0f * (mx / 1280.0f - 0.5f);
            float y = 2.0f * (my / 720.0f - 0.5f);

            for (Door* d: current_area->doors) {
                if (d->position.x - x > -d->size.x && d->position.x - x < d->size.x && d->position.y - y > -d->size.y && d->position.y - y < d->size.y) {
                    d->interact();
                    return true;
                }
            }

            for (Interactable* d: current_area->keys) {
                if (d->position.x - x > -d->size.x && d->position.x - x < d->size.x && d->position.y - y > -d->size.y && d->position.y - y < d->size.y) {
                    d->interact();
                    return true;
                }
            }
        }
        return true;
    }

	return false;
}

void PlayMode::update(float elapsed) {
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

    dialog_time += elapsed * 20.0f;
}

void draw_dialog() {
    if (!dialogs.empty()) {
        std::string s = dialogs[0];
        draw_image(image_vao, dialog_tex, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), -1.0f, -0.75f, 2.0f, 0.25f);
        draw_string(s, glm::vec4(0.9f, 0.8f, 1.0f, 1.0f), -0.9f, -0.75f, 0.025f, 0.1f);
    } else
        dialog_time = 0.0f;
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

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

    draw_image(image_vao, scenes[current_area->texture], glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 0.0f, 1.0f, 1.0f);
    draw_dialog();
    //draw_image(image_vao, font_tex, glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), 0.1f, 0.1f, 0.1f, 0.1f);

	GL_ERRORS();
}
