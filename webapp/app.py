from flask import Flask, render_template, request, abort, make_response, redirect, url_for, jsonify
from random import randint
import requests
import json
import jwt
from base64 import b64decode
import mysql.connector
import paho.mqtt.client as mqtt
import time

app = Flask(__name__, static_url_path='')

values = []

temp = 0

@app.route('/', methods = ['GET'])
def index():
    return render_template('index.html')

@app.route('/home', methods = ['GET'])
def home():
	return render_template('home.html')

@app.route('/monitor', methods = ['GET'])
def monitor():
	return render_template('monitor.html')

# def on_message(client, userdata, message):
#     global values
#     bot = telegram.Bot(token=token)
#     # print("message qos=",message.qos)
#     # print("message retain flag=",message.retain)
#     values.append(float(message.payload.decode("utf-8")))
#     print("BOH" + str(message.payload.decode("utf-8")))

def on_connect(client, userdata, flags, rc):
	print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
	client.subscribe("eventTopic/monitor/values")
	client.subscribe('eventTopic/temperature/value')


# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
	global temp
	global values

	print(msg.topic)
	if (msg.topic == "eventTopic/temperature/value"):
		temp = int(msg.payload.decode("utf-8"))
		print(temp)
	else:
		value = float(msg.payload.decode("utf-8"))
		if (value > 10.0):
			values.append(value)
		print("appended: " + str(value))

@app.route('/monitor/start')
def start_monitoring():
	client = mqtt.Client()

	client.on_message = on_message

	client.connect("127.0.0.1", 1883)
	client.subscribe('eventTopic/monitor/values')
	time.sleep(0.5)
	print("sono dentro monitoring")
	client.publish('eventTopic/monitor/start', 'start')
	time.sleep(4)

	client.loop_start()
	time.sleep(20)
	client.loop_stop()
	
	global values
	if (len(values) == 0):
		return str(0), 404
	avg = sum(values) / len(values)
	values = []
	print(avg)
	return str(int(avg)), 200

@app.route('/temperature/start')
def start_temperature():
	client = mqtt.Client()

	client.on_message = on_message

	client.connect("127.0.0.1", 1883)
	client.subscribe('eventTopic/temperature/value')
	time.sleep(0.5)
	print("sono dentro temp")

	client.publish('eventTopic/temperature/start', 'start')
	
	global temp

	while (temp == 0):
		client.loop();
	# client.loop_start()
	# time.sleep(20)	
	# client.loop_stop()
	
	if (temp != 0):
		res = str(temp)
		temp = 0;
		return res, 200
	else:
		return str(0), 404


@app.route('/update', methods = ['GET'])
def update():

	mydb = mysql.connector.connect(
	  host="localhost",
	  user="root",
	  passwd="root",
	  database="test_arduino"
	)

	mycursor = mydb.cursor()
	
#	mycursor.execute("SELECT MIN(id) FROM hello_sensor")
#	minimum = mycursor.fetchone()[0];

	mycursor.execute("SELECT MAX(id) FROM hello_sensor;")
	maximum = mycursor.fetchone()[0];
	print(maximum)

#	nextOne = randint(minimum, maximum)
	mycursor.execute("SELECT temperature, humidity, sound, flame, light, signal_quality, alarm FROM hello_sensor WHERE id=" + str(maximum))
	entry = mycursor.fetchone()

	print(entry)
	row_headers=[x[0] for x in mycursor.description]
	json = dict(zip(row_headers,entry))
	mycursor.close()
	return jsonify(json);

if __name__ == '__main__':
    app.run(debug=True)
