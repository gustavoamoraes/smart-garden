# smart-garden

The purpose of this project was to automate the watering of my plants and log all the sensors data. With that make a online control panel with a cool graph.
Everything is on a ESP32-WROOM, it handles all the sensors and also hosts the web server and database (SQLite). What is being measured is: air humidity, soil mosture, light levels and air temperature. For now I only implemented a manual watering system where the user has to request it on the web page, the microcontroller then turn a relay on for the specified time.

The graph is made with vanilla JavaScript and it shows the data of the four different sensors.

## Panel
![last1](https://user-images.githubusercontent.com/69619969/155453539-5675fd84-1e0c-4d80-887e-5eec1f6884b0.png)

## Real-World Data
Values taken over the course of a day with a interval of 2 minutes. 

![graph2](https://user-images.githubusercontent.com/69619969/155453542-9af24850-ce8c-42e3-919b-681f8c5133ed.png)