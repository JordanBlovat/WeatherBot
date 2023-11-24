#include "MyBot.h"
#include <iostream>
#include <dpp/dpp.h>
#include <json/json.h>
#include <curl/curl.h>
#include <cmath>
#pragma execution_character_set("utf-8")

const std::string    BOT_TOKEN = ""; // Bot token
const std::string    WEATHER_TOKEN = ""; // Token for openweathermap

// Write the callback of a call
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

int main()
{
	// Create bot
	dpp::cluster bot(BOT_TOKEN);

	// Log messages
	bot.on_log(dpp::utility::cout_logger());

	// Handle slash commands
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {
		if (event.command.get_command_name() == "weather") {

            std::string location = std::get<std::string>(event.get_parameter("location"));

            for (size_t i = 0; i < location.length(); ++i) {
                if (location[i] == ' ') {
                    location[i] = '+';
                }
            }

            // Construct the API URL
            std::string apiUrl = "http://api.openweathermap.org/data/2.5/weather?q=" + location + "&appid=" + WEATHER_TOKEN + "&units=imperial";

            // Initialize cURL
            CURL* curl = curl_easy_init();

            if (curl) {
                // Set the API URL
                curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());

                // Set the callback function to handle the response
                std::string response;
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

                // Perform the request
                CURLcode res = curl_easy_perform(curl);

                // Check for errors
                if (res != CURLE_OK) {
                    std::cerr << "cURL failed: " << curl_easy_strerror(res) << std::endl;
                }
                else {
                    // Parse the JSON response
                    Json::Value root;
                    Json::Reader reader;
                    bool parsingSuccessful = reader.parse(response, root);

                    if (parsingSuccessful) {

                        if (root["cod"] == "404") {
                            event.reply("No area with that name found.");
                        }
                        else {
                            std::string weather;

                            // Add an emoji that's reactive to the weather
                            std:int type = root["weather"][0]["id"].asInt() / 100;
                            switch (type) {
                            case 2: // Thunderstorm
                                weather = "⛈";
                                break;
                            case 3: // Drizzle
                                weather = "🌦";
                                break;
                            case 5: // Rain
                                weather = "🌧";
                                break;
                            case 6: // Snow
                                weather = "🌨";
                                break;
                            case 7: // Fog etc.
                                weather = "🌫";
                                break;
                            case 8: // Clear or clouds
                                if (root["weather"][0]["id"].asInt() == 800)
                                    weather = "☀"; // Clear
                                else
                                    weather = "☁"; // Cloudy
                            }

                            // Capitalize the first letter, since the API doesn't do it automatically
                            std::string desc = root["weather"][0]["description"].asString();
                            desc[0] = std::toupper(desc[0]);
                            // Add the weather
                            weather += " Weather: " + desc;

                            // Add temperature
                            double temperature = root["main"]["temp"].asDouble();
                            int temp = round(temperature);
                            weather += "\n🌡️ Temperature: " + std::to_string(temp) + "F";

                            // Create the embed
                            dpp::embed embed = dpp::embed().
                                set_color(0x11aaff).
                                set_title("Weather for " + root["name"].asString()).
                                set_description(weather).
                                set_footer(dpp::embed_footer().set_text("Requested by " + event.command.usr.global_name).set_icon(event.command.usr.get_avatar_url())).
                                set_timestamp(time(0));

                            // Reply to the message
                            event.reply(dpp::message(event.command.channel_id, embed).set_reference(event.command.message_id));
                        }
                    }
                    else {
                        std::cerr << "Failed to parse JSON response" << std::endl;
                    }
                }

                // Clean up
                curl_easy_cleanup(curl);
            }
		}
	});

	// Register slash commands here
	bot.on_ready([&bot](const dpp::ready_t& event) {
        // Making it run once makes sure it doesn't re-run on every connection
		if (dpp::run_once<struct register_bot_commands>()) {
            dpp::slashcommand weather;
            weather.set_name("weather")
                .set_description("Get the weather for a location")
                .set_application_id(bot.me.id)
                .add_option(dpp::command_option(dpp::co_string, "location", "Location", true));

            bot.global_command_create(weather);
		}
	});

    // Start the bot
	bot.start(dpp::st_wait);

	return 0;
}
