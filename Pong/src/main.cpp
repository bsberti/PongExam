#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../include/util.hpp"
#include "../include/ball.hpp"
#include <cstdio>
#include <cstdlib>

#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <FMOD/fmod.hpp>
#include "FModManager.h"

int player_score = 0;
int ai_score = 0;

bool p1u, p1d;

// Where it's at, yo
float p1y;
float p1i = 2.0f; //player speed

float cpy, cpi;

FModManager* fmod_manager;
constexpr int max_channels = 255;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		p1u = true;
	}
	if (key == GLFW_KEY_W && action == GLFW_RELEASE)
	{
		p1u = false;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		p1d = true;
	}
	if (key == GLFW_KEY_S && action == GLFW_RELEASE)
	{
		p1d = false;
	}
}


bool DisplayChannelVolume(std::string channelName)
{
	FModManager::ChannelGroup* channel_group;
	if (!fmod_manager->find_channel_group(channelName, &channel_group)) {
		return false;
	}

	float currentVolume;
	if (!fmod_manager->get_channel_group_volume(channelName, &currentVolume)) {
		return false;
	}

	currentVolume *= 100;
	ImGui::SliderFloat((channelName + " Volume").c_str(), &currentVolume, 0.0f, 100.0f, "%.0f");
	currentVolume /= 100;

	if (!fmod_manager->set_channel_group_volume(channelName, currentVolume)) {
		return false;
	}

	bool volume_enabled;
	if (!fmod_manager->get_channel_group_enabled(channelName, &volume_enabled)) {
		return false;
	}

	ImGui::SameLine();
	ImGui::Checkbox(("##" + channelName + "_volume").c_str(), &volume_enabled);

	if (!fmod_manager->set_channel_group_enabled(channelName, volume_enabled)) {
		return false;
	}

	return true;
}

int main(void)
{
	
	util::Util util;
	pong::Ball ball = pong::Ball(util);

	//set player/computer paddle position
	p1y = 0;
	cpy = 0;

	//set computer speed
	cpi = 1.0f; 

	if (!glfwInit())
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 600, "Pong", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		return -1;
	}
	glfwSetKeyCallback(window, key_callback);

	//initialize imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	if (!ImGui_ImplGlfw_InitForOpenGL(window, true) || !ImGui_ImplOpenGL3_Init("#version 460"))
	{
		return -1;
	}
	ImGui::StyleColorsDark();

	//TODO:
	//intitalize fmod system 
	// ------------------ FMOD INITIALIZATION ------------------------------------
	//initialize fmod with max channels
	FMOD_TAG tag;
	fmod_manager = new FModManager();
	if (!fmod_manager->Initialize(max_channels, FMOD_INIT_NORMAL)) {
		std::cout << "Failed to initialize FMod" << std::endl;
		return -1;
	}

	// 1 - MP3 Format
	// 2 - WAV Format
	fmod_manager->choosenAudio = 1;
	// Load XML soundList and create sounds
	if (fmod_manager->LoadSounds() != 0) {
		std::cout << "Failed to load sounds in sounds/sounds.xml" << std::endl;
		return -5;
	}

	//create channel groups
	if (
		!fmod_manager->create_channel_group("master") ||
		!fmod_manager->create_channel_group("music") ||
		!fmod_manager->create_channel_group("fx")
		)
	{
		return -2;
	}

	fmod_manager->set_channel_group_volume("music", 0.01f);
	fmod_manager->set_channel_group_volume("fx", 0.25f);

	//set parents for channel groups
	if (!fmod_manager->set_channel_group_parent("music", "master") ||
		!fmod_manager->set_channel_group_parent("fx", "master")
		)
		return -4;
	// ------------------ FMOD INICIALIZATION ------------------------------------


	//TODO:
	//load sounds to be played during game play
	fmod_manager->play_sound("bg", "music");

	//initialize the ball
	ball.start();

	float last_time = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		const float now_time = glfwGetTime();
		const float delta_time = now_time - last_time;
		last_time = now_time;

		//move the ball
		ball.tick();

		//if the ball goes past the computer (player wins)
		if (ball.x >= 1.4f)
		{
			player_score += 1;
			ball.reset();
			fmod_manager->play_sound("victory", "fx");
		}

		//if the ball goes past the player (computer wins)
		if (ball.x <= -1.4f)
		{
			ai_score += 1;
			ball.reset();
			fmod_manager->play_sound("game-over", "fx");
		}

		//bounce ball off bottom
		if (ball.y >= 1.0f)
		{
			ball.collideUp();
			fmod_manager->play_sound("collideDown", "fx");
		}

		//bounce ball off top
		if (ball.y <= -1.0f)
		{
			ball.collideDown();
			fmod_manager->play_sound("collideUp", "fx");
		}

		//move the player paddle
		if (p1u && p1y <= 1)
		{
			p1y += p1i * delta_time;
		}

		if (p1d && p1y >= -1)
		{
			p1y -= p1i * delta_time;
		}


		//move the computer paddle (just try match the balls y position)
		if (ball.y > cpy)
		{
			cpy += cpi * delta_time;
		}

		if (ball.y < cpy)
		{
			cpy -= cpi * delta_time;
		}

		//check for collisions with player paddle
		if ((ball.x <= -1.15f) && (ball.x >= -1.25f) && (ball.y <= p1y + 0.15) && (ball.y >= p1y - 0.15))
		{
			ball.collideRight();
			cpi += util.randFloat(0.001f, 0.005f);
			fmod_manager->play_sound("collideRight", "fx");
		}

		//check for collisions with computer paddle
		if ((ball.x >= 1.15f) && (ball.x <= 1.25f) && (ball.y <= cpy + 0.15) && (ball.y >= cpy - 0.15))
		{
			ball.collideLeft();
			cpi += util.randFloat(0.001f, 0.005f);
			fmod_manager->play_sound("collideLeft", "fx");
		}

		//render
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		const float ratio = width / static_cast<float>(height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Draw player paddle
		glPushMatrix();
		glTranslatef(-1.2f, p1y, 0);
		glScalef(0.05f, 0.05f, 1);
		glBegin(GL_QUADS);
		glColor3f(0.f, 0.0f, 0.5f);
		glVertex3f(-0.5f, -2.0f, 0.0f);
		glVertex3f(-0.5f, 2.0f, 0.0f);
		glVertex3f(0.5f, 2.0f, 0.0f);
		glVertex3f(0.5f, -2.0f, 0.0f);
		glEnd();
		glPopMatrix();

		// Draw ball
		glPushMatrix();
		glTranslatef(ball.x, ball.y, 0);
		glScalef(0.05f, 0.05f, 1);
		glBegin(GL_QUADS);
		glColor3f(0.5f, 0.0f, 0.0f);
		glVertex3f(-0.5f, -0.5f, 0.0f);
		glVertex3f(-0.5f, 0.5f, 0.0f);
		glVertex3f(0.5f, 0.5f, 0.0f);
		glVertex3f(0.5f, -0.5f, 0.0f);
		glEnd();
		glPopMatrix();

		// Draw AI paddle
		glPushMatrix();
		glTranslatef(1.2f, cpy, 0);
		glScalef(0.05f, 0.05f, 1);
		glBegin(GL_QUADS);
		glColor3f(0.f, 0.5f, 0.0f);
		glVertex3f(-0.5f, -2.0f, 0.0f);
		glVertex3f(-0.5f, 2.0f, 0.0f);
		glVertex3f(0.5f, 2.0f, 0.0f);
		glVertex3f(0.5f, -2.0f, 0.0f);
		glEnd();
		glPopMatrix();


		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//score window
		ImGui::Begin("Current Score");
		ImGui::Text("Player:\n%d", player_score);
		ImGui::SameLine();
		ImGui::Text("AI:\n%d", ai_score);
		ImGui::End();

		//audio settings window
		ImGui::Begin("Audio Settings");
		//your controls here (or as keyboard commands above in callback)
		ImGui::BulletText("Master Channel");
		if (!DisplayChannelVolume("master")) {
			// Something went wrong, what now?
		}

		ImGui::BulletText("Music Channel");
		if (!DisplayChannelVolume("music")) {
			// Something went wrong, what now?
		}

		ImGui::BulletText("Sound Effect Channel");
		if (!DisplayChannelVolume("fx")) {
			// Something went wrong, what now?
		}
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	fmod_manager->Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}