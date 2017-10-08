import os
import subprocess
import numpy as np
import pickle

def meter_caller():
    FNULL = open(os.devnull, 'w')    #use this if you want to suppress output to stdout from the subprocess

    args = "rou5.exe -c COM6"
    prev_transient = None
    none_flag = True
    
    while True:
        process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=FNULL, shell=True)
        raw_output = str(process.stdout.read())
        output_seq = raw_output.split(' ')[:-1][1::2]
        test_set = [int(output_seq[i]) for i in (1, 2, 3, 4, 6, 8)]        
        pickle_in = open('smart_meter_svm_model.pickle','rb')
        clf = pickle.load(pickle_in)

        if test_set[0] < 2:
            prev_transient = output_seq[0]
            none_flag = True
            yield 'None'
            continue
        if none_flag:
            if prev_transient is None:
                prev_transient = output_seq[0]
                yield 'None'
                continue               
            if output_seq[0] > prev_transient:
                prev_transient = output_seq[0]
                none_flag = False
                yield 'None'
                continue
        prev_transient = output_seq[0]
        yield ''.join(clf.predict(np.array([test_set])))

if __name__ == '__main__':
    smart_meter_iter = meter_caller()
    while True:
        print(next(smart_meter_iter))