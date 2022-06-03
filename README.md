# CS147 Final Project - AlarmingData

Our project aims to track the environmental factors of temperature, humidity, and light that affect the user’s waking up experience. With a functioning alarm clock and data visualizations, the user will be able to view how the environment may affect how they wake up and adjust accordingly.

Our project utilizes the LILYGO-TTGO ESP32 board to display the alarm clock and collect the sensor data from the humidity and temperature sensor as well as the light sensor. A buzzer is also connected to play an alarm, and a button is used to stop the alarm. All the sensor data is sent through HTTP protocol through Wi-Fi to an AWS server, where the data is then visualized on multiple Flask websites. Lastly, the user is able to set the alarm by visiting the Flask website, and that time is then sent to the ESP32 board through Wi-Fi.

Overall, our project fulfills its goal of being a functional alarm clock with additional features of tracking and visualizing environmental data. 
