token = '--Insert your token here--'

from telegram.ext import Updater
import paho.mqtt.client as mqtt
import logging
from telegram.ext import CommandHandler
import telegram

chat_id = ''
selected_topic = ''
shock_msg='Shock detected'
tilt_msg='Tilt detected'
alarm_msg='Assistance required'
topic = 'eventTopic'

def start(bot, update):
    """Send a message when the command /start is issued."""
    bot.send_message(chat_id=update.message.chat_id, text='The bot started')
    global chat_id
    chat_id = update.message.chat_id
    print(chat_id)
    global selected_topic
    selected_topic = topic
    

def stop(bot, update):
    """Send a message when the command /start is issued."""
    bot.send_message(chat_id=update.message.chat_id, text='The bot stopped')
    global selected_topic
    selected_topic = ''
    global chat_id
    chat_id = ''

def heartbeat(bot, update):
    """Send a message when the command /start is issued."""
    global selected_topic
    selected_topic = topic

def shock(bot, update):
    """Send a message when the command /start is issued."""
    global selected_topic
    selected_topic = shock_data

def tilt(bot, update):
    global selected_topic
    """Send a message when the command /start is issued."""
    selected_topic = tilt_data

def send_message(bot,chat_id,message):
    bot.send_message(chat_id=chat_id, text=message)


def on_message(client, userdata, message):
    msg = str(message.payload.decode("utf-8"))
    print("message received: ", msg)
    global selected_topic
    bot = telegram.Bot(token=token)
    # print("message qos=",message.qos)
    # print("message retain flag=",message.retain)
    if(chat_id != ''):
        if(msg == 'shock'):
            bot.send_message(chat_id=chat_id, text=shock_msg)
        elif (msg == 'tilt'):
            bot.send_message(chat_id=chat_id, text=tilt_msg)
        else:
            bot.send_message(chat_id=chat_id, text=alarm_msg)
    else:
        print("Nessun topic")



def setup():

    client_name = "arduino"
    client = mqtt.Client()
    client.connect("127.0.0.1",port=1883)

    client.subscribe(topic)
    print("Subscribed")

    client.on_message = on_message
    client.loop_start()


    # The goal is to have this function called every time the Bot receives a Telegram message that
    # contains the /start command. To accomplish that, you can use a CommandHandler
    # (one of the provided Handler subclasses) and register it in the dispatcher:
    updater = Updater(token=token)
    updater
    dispatcher = updater.dispatcher

    logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                        level=logging.INFO)

    start_handler = CommandHandler('start', start)
    dispatcher.add_handler(start_handler)

    stop_handler = CommandHandler('stop', stop)
    dispatcher.add_handler(stop_handler)

    # And that's all you need. To start the bot, run:
    updater.start_polling()
    bot = telegram.Bot(token=token)


setup()









