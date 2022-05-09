from Adafruit_IO import Client, Feed, Data, RequestError
import datetime
import requests
import time

ADAFRUIT_IO_KEY = 'aio_oaTI20dnUkujjKtdYfIjSM4jgOpm'
ADAFRUIT_IO_USERNAME = 'itssimsch'


aio = Client(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)
try:
    temperature = aio.feeds('temperature')
except RequestError:
    feed = Feed(name="temperature")
    temperature = aio.create_feed(feed)

while True:
    r = requests.get("https://10.0.0.35:9443/Ny8KcFDRPdWDGifJjUZ9ZlFeRee2ih28/get/V0", verify=False)
    aio.send_data(temperature.key, float(r.text[2:-2]))
    time.sleep(2)