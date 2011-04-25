# 
# test_print_first.py
# 
# Copyright 2008 Helsinki Institute for Information Technology (HIIT)
# and the authors. All rights reserved.
# 
# Authors: Tero Hasu <tero.hasu@hut.fi>
# 

# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import pyinbox

def read_file(fname):
    fp = open(fname, "r")
    try:
        return fp.read()
    finally:
        fp.close()

max_read = 1024

def read_data(inbox, msg_id):
    datalen = inbox.size(msg_id)
    read = 0
    data = []
    while read < datalen:
        thismax = datalen - read
        if thismax > max_read:
            thismax = max_read
        s = inbox.data(msg_id, read, thismax)
        read += len(s)
        data.append(s)
    return "".join(data)

def print_message(msg_id):
    print("message %d" % msg_id)
    
    if inbox.unread(msg_id):
        print "unread"
    else:
        print "read"

    print("message type is %x" % inbox.message_type(msg_id))
    print("message description is " + inbox.description(msg_id))

    fname = None
    try:
        fname = inbox.attachment_path(msg_id)
    except:
        # Will not get the path on v9.
        pass

    if fname:
        fname = inbox.attachment_path(msg_id)
        print(repr(fname))
        print(repr(read_file(fname)))
        print(repr(inbox.address(msg_id))) # always prints "Bluetooth", presumably
    else:
        datalen = inbox.size(msg_id)
        print("size is %d" % datalen)
        print(repr(read_data(inbox, msg_id)))

inbox = pyinbox.Inbox(0x10009ED5)
m = inbox.list_messages(0x10009ED5)
if len(m) > 0:
    print_message(m[0])
else:
    print "no BT OBEX messages"
