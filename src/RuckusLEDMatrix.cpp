#include"RuckusLEDMatrix.h"

/// @brief Creates a RoboRuckus LED matrix controller
/// @param Name The device name
/// @param Pin Pin to use
/// @param LED_X Number of LEDs on x-axis of matrix
/// @param LED_Y Number of LEDs on y-axis of matrix
/// @param RGB_Type The type of NeoPixel
/// @param configFile Name of the config file to use
RuckusLEDMatrix::RuckusLEDMatrix(String Name, int Pin, int LED_X, int LED_Y, neoPixelType RGB_Type, String configFile) : NeoPixelsController(Name, Pin, LED_X * LED_Y, RGB_Type, configFile) {
	display_config.LED_X = LED_X;
	display_config.LED_Y = LED_Y;
}

/// @brief Starts a RoboRuckus LED matrix controller
/// @return True on success
bool RuckusLEDMatrix::begin() {
	bool result = false;
	bool configExists = checkConfig(config_path);
	if (NeoPixelsController::begin()) {
		// Set description
		Description.type = "display";
		Description.version = "1.0.0";
		ip_action = Description.actions.size();
		Description.actions["showIP"] = ip_action;
		if (!configExists) {
			// Set defaults
			result = setConfig(getConfig(), true);
		} else {
			// Load settings
			result = setConfig(Storage::readFile(config_path), false);
		}
		if (result) {
			eventPayload clear = {.event = CUSTOM, .eventType = (int)CLEAR};
			receiveEvent(&clear);
		}

	}
	return result;
}

/// @brief Receives an action
/// @param action The action to process 0 to set colors 1 to show IP
/// @param payload Either an array or RGB(W) values, or a brightness value 0-255
/// @return JSON response with OK
std::tuple<bool, String> RuckusLEDMatrix::receiveAction(int action, String payload) {
	if (action == ip_action) {
		showIP();
		return { true, R"({"success": true})" }; 
	} else {
		return NeoPixelsController::receiveAction(action, payload);
	}
}

/// @brief Gets the current config
/// @return A JSON string of the config
String RuckusLEDMatrix::getConfig() {
	JsonDocument doc = addAdditionalConfig();
	// Create string to hold output
	String output;
	// Serialize to string
	serializeJson(doc, output);
	return output;
}

/// @brief Sets the configuration for this device
/// @param config A JSON string of the configuration settings
/// @param save If the configuration should be saved to a file
/// @return True on success
bool RuckusLEDMatrix::setConfig(String config, bool save) {
	if (NeoPixelsController::setConfig(config, false)) {
		// Allocate the JSON document
		JsonDocument doc;
		// Deserialize file contents
		DeserializationError error = deserializeJson(doc, config);
		// Test if parsing succeeds.
		if (error) {
			Logger.print(F("Deserialization failed: "));
			Logger.println(error.f_str());
			return false;
		}
		// Assign loaded values
		// Parse RGB color values
		String colorString = doc["color"].as<String>();
		display_config.color.clear();
		int commaCount = 1;
		for(int i = 0; i < colorString.length(); i++) {
			if(colorString[i] == ',') commaCount++;
		}
		display_config.color.resize(commaCount);
		int startIndex = 0;
		int commaIndex;
		int colorCount = 0;
		while ((commaIndex = colorString.indexOf(',', startIndex)) != -1) {
			display_config.color[colorCount++] = colorString.substring(startIndex, commaIndex).toInt();
			startIndex = commaIndex + 1;
		}
		display_config.color[colorCount++] = colorString.substring(startIndex).toInt();
		eventPayload current = {.event = CUSTOM, .eventType = (int)currentImage};
		receiveEvent(&current);

		if (save) {
			return saveConfig(config_path, config);
		}
		return true;
	}
	return false;
}

/// @brief Used to process an event on its own thread to allow multitasking
/// @param event The eventPayload to process
void RuckusLEDMatrix::processEvent(eventPayload* event) {
	if (event == nullptr) {
		Logger.print("Bad event payload received by: ");
		Logger.println(Description.name);
		return;
	}
	switch(event->event) {
		case NOTREADY:
			showImage(SAD, true);
			break;
		case RESET:
		case READY:
			showImage(HAPPY, true);
			break;
		case ENTERCONFIG:
			showImage(DUCK, false);
			break;
		case TAKEDAMAGE:
			showImage(SURPRISED, false);
			delay(1500);
			showImage(currentImage, false);
			break;
		case ASSIGNPLAYER:
			if (event->magnitude < 10) {
				showImage((images)event->magnitude, true);
			}
			break;
		case EXITCONFIG:
			showImage(currentImage, false);
			break;
		case CUSTOM:
			showImage((images)event->eventType, true); // Directly show an image
		default:
			break;
	}
}

/// @brief Shows an image on the display
/// @param image The image to show
/// @param replace Replace the current image, i.e. set this new image as persistent
void RuckusLEDMatrix::showImage(images image, bool replace) {
	if (replace)
	{
		currentImage = image;
	}
	uint8_t* dat = image_maps5x5[(int)image];
	if (display_config.color.size() == 3) {
		uint8_t matrix[25][3];
		for (int r = 0; r < display_config.LED_X; r++)
		{
			for (int c = 0; c < display_config.LED_Y; c++)
			{
				int index = r * display_config.LED_X + display_config.LED_Y - (c + 1);
				if (bitRead(dat[r], c))
				{	
					matrix[index][0] = display_config.color[0];
					matrix[index][1] = display_config.color[1];
					matrix[index][2] = display_config.color[2];
				} else {
					matrix[index][0] = 0;
					matrix[index][1] = 0;
					matrix[index][2] = 0;
				}
			}
		}
		writePixels(matrix);
	} else {
		uint8_t matrix[25][4];
		for (int r = 0; r < display_config.LED_X; r++)
		{
			for (int c = 0; c < display_config.LED_Y; c++)
			{
				int index = r * display_config.LED_X + display_config.LED_Y - (c + 1);
				if (bitRead(dat[r], c))
				{	
					matrix[index][0] = display_config.color[0];
					matrix[index][1] = display_config.color[1];
					matrix[index][2] = display_config.color[2];
					matrix[index][3] = display_config.color[3];
				} else {
					matrix[index][0] = 0;
					matrix[index][1] = 0;
					matrix[index][2] = 0;
					matrix[index][3] = 0;
				}
			}
		}
		writePixels(matrix);
	}
}

/// @brief Displays the last octet of the robot's IP on the display
void RuckusLEDMatrix::showIP() {
	String last_octet = RuckusCommunicator::Config.robotIP.substring(RuckusCommunicator::Config.robotIP.lastIndexOf('.') + 1);
	Logger.println("Last octet: " + last_octet);
	showImage(CLEAR, false);
	delay(500);
	for (int i = 0; i < last_octet.length(); i++)
	{
		// Substring is used instead of [] operator since the [] operator seems to access the wrong part of memory
		showImage((images)last_octet.substring(i, i + 1).toInt(), false);
		delay(1500);
		showImage(CLEAR, false);
		delay(1000);
	}
	showImage(currentImage, false);
}

/// @brief Collects all the base class parameters and additional parameters
/// @return A JSON document with all the parameters
JsonDocument RuckusLEDMatrix::addAdditionalConfig() {
	// Allocate the JSON document
	JsonDocument doc;
	// Deserialize file contents
	DeserializationError error = deserializeJson(doc, NeoPixelsController::getConfig());
	// Test if parsing succeeds.
	if (error) {
		Logger.print(F("Deserialization failed: "));
		Logger.println(error.f_str());
		return doc;
	}
	String color = "";
	for (int j = 0; j < display_config.color.size(); j++) {
		if (j > 0) {
			color += ",";
		}
		color += String(display_config.color[j]);
	}
	doc["color"] = color;
	return doc;
}
