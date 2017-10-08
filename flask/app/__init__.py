# -*- coding: utf-8 -*-

# Author:  vitos
# File:    __init__.py
# Created: 16.8.2017 15:10

# build-in
import os
import pickle
import subprocess
from time import sleep
from threading import Thread

# numpy
import numpy as np

# flask
from flask import Flask, render_template, url_for, jsonify


# application
app = Flask(__name__, static_folder="static", template_folder="templates")

# config
app.config.from_object("config")

# thread results
global thread_result
thread_result = []


@app.route("/")
def index():

    return render_template("index.html")


@app.route("/start-predictions", methods=["GET"])
def start_predictions():

    t = Thread(target=get_predict_device)
    t.daemon = True
    t.start()

    return jsonify(status="OK", data="")


@app.route("/get-predictions", methods=["GET"])
def get_predictions():

    if thread_result:
        if thread_result[-1] == "Error":
            return jsonify(status="ERROR", device_type="", device_img="/")
        elif thread_result[-1] == "None":
            return jsonify(status="OK", device_type="", device_img="/")
        else:
            device_prediction = thread_result[-1]

            if device_prediction is not None:
                predicted_device_img = app.config["DICT_DEVICES_IMGS"][device_prediction]
                predicted_device_type = app.config["DICT_DEVICES_NAMES"][device_prediction]

                return jsonify(status="OK",
                               device_type=predicted_device_type,
                               device_img='<div class="device-img-wrapper mt-3"><img class="device-img" src=' + url_for("static", filename="media/" + predicted_device_img) + '></div>')
    else:
        return jsonify(status="OK", device_type="", device_img="/")


def get_predict_device():

    args = "D:\\PROJECTS\\HACKATHON\\flask\\app\\rou5.exe -c COM6"

    pickle_in = open('D:\\PROJECTS\\HACKATHON\\flask\\app\\smart_meter_svm_model.pickle', 'rb')
    clf = pickle.load(pickle_in)

    FNULL = open(os.devnull, 'w')

    while True:
        try:
            process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=FNULL, shell=True)
            raw_output = str(process.stdout.read())
            output_seq = raw_output.split(' ')[:-1][1::2]

            test_set = [int(output_seq[i]) for i in (1, 2, 3, 4, 6, 8)]

            if test_set[0] < 2:
                prediction = "None"
            else:
                prediction = ''.join(clf.predict(np.array([test_set])))

        except Exception as e:
            print(e)
            prediction = "Error"

        print("Thread results: " + prediction)

        thread_result.append(prediction)

        sleep(3)
