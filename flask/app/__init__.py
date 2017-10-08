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
thread_result = 0


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

    if thread_result == -1:
        return jsonify(status="ERROR", data="/")
    elif thread_result == 0:
        return jsonify(status="OK", data="/")
    else:
        if thread_result is not None:
            predicted_device_img = app.config["DICT_DEVICES_IMGS"][thread_result]

            return jsonify(status="OK", data='<div class="device-img-wrapper mt-3"><img class="device-img" src=' + url_for("static", filename="media/" + predicted_device_img) + '></div>')


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
                thread_result = 0
            else:
                thread_result = ''.join(clf.predict(np.array([test_set])))

        except Exception as e:
            print(e)
            thread_result = -1

        sleep(3)
