/*
 * This file and associated .cpp file are licensed under the GPLv3 License Copyright (c) 2025 Sam Groveman
 * 
 * External libraries needed:
 * ArduinoJSON: https://arduinojson.org/
 * 
 * Contributors: Sam Groveman
 */
#pragma once
#include <Actor.h>
#include <ArduinoJson.h>
#include <RoboRuckusDevice.h>
#include <NeoPixelsController.h>

/// @brief Class describing a RoboRuckus LED matrix display
class RuckusLEDMatrix : public NeoPixelsController, public RoboRuckusDevice {
public:
	RuckusLEDMatrix(String Name, int Pin, int LED_X = 5, int LED_Y = 5, neoPixelType RGB_Type = NEO_GRB + NEO_KHZ800, String configFile = "RuckusLEDMatrix.json");
	bool begin();
	std::tuple<bool, String> receiveAction(int action, String payload);
	String getConfig();
	bool setConfig(String config, bool save);

protected:
	/// @brief Image maps for display. Binary maps for each row, 1=on, 0=off
	uint8_t image_maps5x5[16][5] = {
		{B01100,B10010,B10010,B10010,B01100}, // 0
		{B00100,B01100,B00100,B00100,B01110}, // 1
		{B11100,B00010,B01100,B10000,B11110}, // 2
		{B11110,B00010,B00100,B10010,B01100}, // 3
		{B00110,B01010,B10010,B11111,B00010}, // 4
		{B11111,B10000,B11110,B00001,B11110}, // 5
		{B00010,B00100,B01110,B10001,B01110}, // 6
		{B11111,B00010,B00100,B01000,B10000}, // 7
		{B01110,B10001,B01110,B10001,B01110}, // 8
		{B01110,B10001,B01110,B00100,B01000}, // 9
		{B01010,B01010,B00000,B10001,B01110}, // Happy
		{B01010,B01010,B00000,B01110,B10001}, // Sad
		{B01010,B00000,B00100,B01010,B00100}, // Surprised
		{B01100,B11100,B01111,B01110,B00000}, // Duck
		{B00000,B00001,B00010,B10100,B01000}, // Check
		{B00000,B00000,B00000,B00000,B00000}  // Clear
	};

	/// @brief Enum of image maps for the screen
	enum images {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, HAPPY, SAD, SURPRISED, DUCK, CHECK, CLEAR};

	struct {
		/// @brief The current color of the display
		std::vector<uint8_t> color {127, 127, 127};
		
		/// @brief The number of LEDs on the X-axis
		int LED_X;
		
		/// @brief The number of LEDs on the Y-axis
		int LED_Y;		
	} display_config;

	/// @brief Stores the index for the showIP acton
	int ip_action = 0;

	/// @brief Stores the currently displayed image
	images currentImage = CLEAR;

	JsonDocument addAdditionalConfig();
	void showImage(images image, bool replace);
	void updateDisplay();
	void showIP();
	void processEvent(eventPayload* event);
};