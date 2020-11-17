#include "raylib.h"
#include"raymath.h"

#include "particle.h"
#include "stopwatch.h"

#include <iostream>
#include <algorithm>
#include <string>
#include <cassert>

#include <thread>
#include <mutex>
#include <atomic>

int main()
{
	// Initialization
	//--------------------------------------------------------------------------------------
	int screenWidth = 800;
	int screenHeight = 450;

	InitWindow(screenWidth, screenHeight, "");



	// Particle System Setup
	const size_t PARTICLE_ARRAY_LENGTH = 1000;
	const size_t PARTICLE_BUFFER_LENGTH = 2;
	particle **particles = new particle * [PARTICLE_BUFFER_LENGTH];

	for (size_t i = 0; i < PARTICLE_BUFFER_LENGTH;  i++)
	{
		particles[i] = new particle[1];
	}

	// Time between particle emissions/spawning in seconds
	float ptclSpawnInterval = 0.001f;

	float ptclSpawnTimer = ptclSpawnInterval;
	size_t ptclCount = 0;

	size_t frontIdx = 0;
	size_t backIdx = 1;

	bool shuttingDown = false;

	int threadCount = 7;

	std::thread* ptclWorkerThreads = new std::thread[threadCount];
	size_t* workloads = new size_t[threadCount];
	bool* newFrame = new bool[threadCount];

	for (size_t i = 0; i < threadCount; i++)
	{
		newFrame[i] = false;
		workloads[i] = 0;

		ptclWorkerThreads[i] = std::thread([&](int threadID)
		{
				while (!shuttingDown)
				{
					// Exit early if not a new frame
					if (!newFrame[threadID]) { continue; }

					int threadLoad = ptclCount / threadCount;
					int startIdx = threadID * threadLoad;

					float curTime = GetTime();
					float delTime = GetFrameTime();

					for (size_t i = 0; i < workloads[threadID]; i++)
					{
						size_t idx = startIdx + i;
						auto& curPtcl = particles[backIdx][idx];
						curPtcl.pos = Vector2Add(particles[frontIdx][idx].pos, Vector2Scale(particles[frontIdx][idx].vel, delTime));
					}

					newFrame[threadID] = false;
				}
		}, i);
	}

	size_t sampleCount = 1;
	float avgMs = 16.0f;
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		stopwatch<std::chrono::steady_clock> cpuTime;

		ptclSpawnTimer += GetFrameTime();

		SetWindowTitle((std::string("Threaded particle system @ CPU: ") + std::to_string(avgMs) + "ms w/ " + std::to_string(threadCount) + " thread(s)").c_str());

		// Wait for all threads to complete
		for (size_t i = 0; i < threadCount; i++)
		{
			while (newFrame[i]) {}
		}

		// swapping front and back values
		std::swap(frontIdx, backIdx);

		for (size_t i = 0; i < ptclCount; i++)
		{
			auto& curPtcl = particles[backIdx][i];
			if (!curPtcl.isAlive(GetTime()))
			{
				std::swap(curPtcl, particles[backIdx][ptclCount - 1]);
				std::swap(particles[frontIdx][i], particles[frontIdx][ptclCount - 1]);

				--ptclCount;
			}
		}



		// "new particles?" - terry lol
		while (ptclSpawnTimer >= ptclSpawnInterval && ptclCount < PARTICLE_ARRAY_LENGTH);
		{
			auto& curPtcl = particles[backIdx][ptclCount];

			// Sub from timer
			// Spawned a new particle :D
			ptclSpawnTimer -= ptclSpawnInterval;

			curPtcl.pos = Vector2{ (float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2 };
			curPtcl.startTime = GetTime();
			curPtcl.size = GetRandomValue(15.0f, 45.0f);
			curPtcl.color = ColorFromHSV
			(
				Vector3
				{
					(float)GetRandomValue(0,360), // Sat
					(float)GetRandomValue(0,100), // Hue
					(float)GetRandomValue(0,100)  // Value
				}
			);
			curPtcl.lifeTime = GetRandomValue(5.0f, 50.0f);

			curPtcl.vel = Vector2Scale(Vector2Normalize(
				Vector2
				{
					(float)GetRandomValue(-1.0f, 1.0f),
					(float)GetRandomValue(-1.0f, 1.0f)
				}
			), GetRandomValue(50.0f, 100.0f));

			particles[frontIdx][ptclCount] = curPtcl;

			++ptclCount;
		}

		for (size_t i = 0; i < threadCount; i++)
		{
			workloads[i] = ptclCount / threadCount;
		}
		workloads[threadCount - 1] += ptclCount % threadCount;

		for (size_t i = 0; i < threadCount; i++)
		{
			newFrame[i] = true;
		}

		auto cpuMeasure = std::chrono::duration_cast<std::chrono::milliseconds>(cpuTime.tick());
		avgMs += (cpuMeasure.count() - avgMs) / sampleCount;
		++sampleCount;

		BeginDrawing();

		ClearBackground(RAYWHITE);

		for (size_t idx = 0; idx < ptclCount; idx++)
		{
			particles[frontIdx][idx].draw();
		}

		EndDrawing();
	}
	CloseWindow();        // Close window and OpenGL context
	shuttingDown = true;
	for (size_t i = 0; i < threadCount; i++)
	{
		ptclWorkerThreads[i].join();
	}

	for (size_t i = 0; i < PARTICLE_BUFFER_LENGTH; i++)
	{
		delete[] particles[i];
	}
	delete[] particles;

	return 0;
}