import time

from flask import Flask, request
from flask_cors import CORS


app = Flask(__name__)
CORS(app)

latitude = None
longitude = None
ts = None


@app.route('/')
def hello_world():
    return 'Hello, World!'


@app.route('/location')
def get_location():
    if ts and ts > time.time() - 1:
        return {'latitude': latitude, 'longitude': longitude}
    return {}


@app.route('/location', methods=['POST'])
def post_location():
    global latitude, longitude, ts
    body = request.get_json()
    latitude = body['latitude']
    longitude = body['longitude']
    ts = time.time()
    return 'ok'
