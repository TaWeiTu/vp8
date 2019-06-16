#! /usr/bin/env python3
import os
import hashlib

prefix = os.environ['VP8_TEST_VECTORS']
if prefix[-1] != '/': 
    prefix += '/'
binary = './decode'

test = []

with open(prefix + 'test_case_14xx_descriptions.tsv') as f:
    for line in f.readlines():
        dat = line.split('\t')
        #  if dat[1].startswith('vp8_inter'):
        # if dat[1] == 'vp8_inter':
        # if dat[0] == 'vp80-05-sharpness-1428.ivf':
        test.append((dat[0], int(dat[3]), int(dat[4]), int(dat[5])))

for file, frame, height, width in test:
    print('[Test] Testing %s' % file)
    os.system('%s %s tmp_test.yuv' % (binary, prefix + file))
    hashvalue = []
    with open(prefix + file + '.md5') as f:
        for line in f.readlines():
            dat = line.split()
            hashvalue.append(dat[0])

    with open('tmp_test.yuv', 'rb') as f: 
        for fr in range(frame):
            y_byte = []
            u_byte = []
            v_byte = []
            for i in range(height * width):
                b = f.read(1)
                assert b != ''
                y_byte.append(b)
            for i in range(height // 2 * width // 2):
                b = f.read(1)
                assert b != ''
                u_byte.append(b)
            for i in range(height // 2 * width // 2):
                b = f.read(1)
                assert b != ''
                v_byte.append(b)

            with open('tmp_split.yuv', 'wb') as f2:
                for i in y_byte: f2.write(i)
                for i in u_byte: f2.write(i)
                for i in v_byte: f2.write(i)

            md5 = hashlib.md5()
            with open('tmp_split.yuv', 'rb') as f2:
                x = f2.read()
                md5.update(x)

            h = md5.hexdigest()
            if h != hashvalue[fr]:
                print(file, fr, h, hashvalue[fr])
                print('failed')
                break


