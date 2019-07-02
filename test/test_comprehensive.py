#! /usr/bin/env python3
import os
import hashlib

prefix = os.environ['VP8_TEST_VECTORS']
if prefix[-1] != '/': 
    prefix += '/'
binary = './decode'

test = []

temp = 'vp80-00-comprehensive-%03d.ivf'

size = [(176, 144), (176, 144), (176, 144), (176, 144), (176, 144), (175, 143), (176, 144), (1432, 888), (176, 144), (320, 240), (176, 144), (176, 144), (176, 144), (175, 143), (320, 240), (176, 144), (176, 144)]

frames = [29, 49, 49, 29, 49, 48, 29, 2, 49, 57, 29, 29, 29, 49, 260, 29, 29]

print(len(size), len(frames))

for i in range(1, 18):
    f = temp % i
    test.append((f, frames[i - 1], size[i - 1][0], size[i - 1][1]))

passed = True

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
                print('width = %d height = %d' % (width, height))
                passed = False
                break

if not passed: exit(1)


