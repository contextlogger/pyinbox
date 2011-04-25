/**
 * ====================================================================
 *  inboxmodule.cpp
 *
 *  Python API to inbox/SMS handling, partly modified from "RemoteCam" and 
 *  Series_60_Platform_1st_2nd_Edition_SMS_Example_v1_0.zip available from
 *  Forum Nokia. Also, see:
 *  http://discussion.forum.nokia.com/forum/showthread.php?s=
 *  3edbb43477afe2cfc331c62c5a421bf1&threadid=18332
 *
 *  Implements currently following Python type and function:
 * 
 *  Inbox
 *   bind(callable)
 *     Bind this instance to the device global inbox, callable function 
 *     as parameter (with one argument, the id of the message received).
 *     Note that the new message received might not be of type SMS.
 *   delete(id)
 *     Delete the SMS with id from inbox.
 *   unicode address(id)
 *     The address of the SMS with id.
 *   unicode text(id)
 *     The content of the SMS with id. If invoked with non-SMS id empty
 *     unicode u'' will be returned.
 *   int time(id)
 *     The time of arrival (float) of the SMS with id.
 *   bool unread(id)
 *     The status (boolean, 1=unread, 0=read) of the SMS with id.
 *   list<ids> list_messages(msgType)
 *     Returns a list of message IDs in Inbox.
 *
 *  id in all the above methods is of type int.
 *
 *  delete, address, text, time, and unread fail with:
 *    SymbianError: [Errno -1] KErrNotFound
 *  if invoked with non-SMS id (e.g. received MMS id).  
 *
 *  The above is also raised if the constructor is called with unknown
 *  folder identifier.
 *
 * TODO
 *    - Wrapper: "Inbox"-type should be separate if the connection to 
 *    server is lost
 *    - Support for other types of messages (MMS, e-mail) than SMS
 *    - There should be ONLY one CMsvSession in thread -> implement this
 *    to wrapper?
 *
 * Copyright (c) 2005 - 2007 Nokia Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Some changes to this file were made at HIIT. The changes are
 * Copyright 2008 Helsinki Institute for Information Technology (HIIT)
 * and Tero Hasu <tero.hasu@hut.fi>, and are likewise covered by
 * the above license.
 * ====================================================================
 */

#include "inbox.h"


//////////////TYPE METHODS////////////////////

/*
 * Deallocate inb.
 */
extern "C" {

  static void inb_dealloc(INB_object *inbo)
  {
    if (inbo->inbox) {
      delete inbo->inbox;
      inbo->inbox = NULL;
    }
    if (inbo->callBackSet) {
      Py_XDECREF(inbo->myCallBack.iCb);
    }
    PyObject_Del(inbo);
  }

}

/*
 * Allocate inb.
 */
extern "C" PyObject *
new_inb_object(PyObject* /*self*/, PyObject *args)
{
  TMsvId folderType = KMsvGlobalInBoxIndexEntryId;
  TInt error = KErrNone; 

  INB_object *inbo = PyObject_New(INB_object, INB_type);
  if (inbo == NULL)
    return PyErr_NoMemory();

  TInt msgTypeInt;
  if (!PyArg_ParseTuple(args, "i|i", &msgTypeInt, &folderType))  
    return NULL;
  TUid msgType;
  msgType.iUid = msgTypeInt;

  Py_BEGIN_ALLOW_THREADS
  TRAP(error, inbo->inbox = CInboxAdapter::NewL(msgType, folderType));
  Py_END_ALLOW_THREADS

  if (inbo->inbox == NULL) {
    PyObject_Del(inbo);
    return PyErr_NoMemory();
  }

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  inbo->callBackSet = EFalse;

  return (PyObject*) inbo;
}

/*
 * Bind the call back for Inbox notifications.
 */
extern "C" PyObject *
inb_bind(INB_object* self, PyObject *args)
{
  PyObject* c;
  
  if (!PyArg_ParseTuple(args, "O:set_callback", &c))
    return NULL;
  
  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  Py_XINCREF(c);
  
  self->myCallBack.iCb = c;
  self->callBackSet = ETrue;

  self->inbox->SetCallBack(self->myCallBack);

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Delete message.
 */
extern "C" PyObject *
inb_delete(INB_object* self, PyObject *args)
{
  TInt message_id = 0;

  if (!PyArg_ParseTuple(args, "i", &message_id))
    return NULL;

  TRAPD(error, self->inbox->DeleteMessageL((TMsvId)message_id))

  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Get message address.
 */
extern "C" PyObject *
inb_address(INB_object* self, PyObject *args)
{
  TInt message_id = 0;

  if (!PyArg_ParseTuple(args, "i", &message_id))
    return NULL;

  TBuf<KMessageAddressLength> address;

  TRAPD(error, self->inbox->GetMessageAddressL((TMsvId)message_id, address))
  
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("u#", address.Ptr(), address.Length());
}

/*
 * Get message text.
 */
extern "C" PyObject *
inb_text(INB_object* self, PyObject *args)
{
  TInt message_id = 0;

  if (!PyArg_ParseTuple(args, "i", &message_id))
    return NULL;

  TBuf<KMessageBodySize> text;

  TRAPD(error, self->inbox->GetMessageL((TMsvId)message_id, text))
  
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("u#", text.Ptr(), text.Length());
}

/*
 * Converts a Symbian Universal Time value as TTime into a Python
 * float representing the equivalent Unix time.
 */
PyObject *
SPyUnixTime_FromSymbianUniversalTime(TTime& aUniversalTime)
{
  TTime unixEpoch(TDateTime(1970,EJanuary,0,0,0,0,0));
  TInt64 unixTimeInMicroseconds = 
    aUniversalTime.MicroSecondsFrom(unixEpoch).Int64();
#ifdef EKA2
  double unixTime = TReal64(unixTimeInMicroseconds) / 1000000.;
#else
  double unixTime = unixTimeInMicroseconds.GetTReal() / 1000000.;
#endif  
  return PyFloat_FromDouble(unixTime);
}

/*
 * Get message time of arrival.
 */
extern "C" PyObject *
inb_time(INB_object* self, PyObject *args)
{
  TInt message_id = 0;

  if (!PyArg_ParseTuple(args, "i", &message_id))
    return NULL;
  
  TTime arrival;
  
  TRAPD(error, self->inbox->GetMessageTimeL((TMsvId)message_id, arrival));
  
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return SPyUnixTime_FromSymbianUniversalTime(arrival);
}

/*
 * Get message read/unread status.
 */
extern "C" PyObject *
inb_unread(INB_object* self, PyObject *args)
{
  TInt message_id = 0;

  if (!PyArg_ParseTuple(args, "i", &message_id))
    return NULL;

  TBool unread;

  TRAPD(error, self->inbox->GetMessageUnreadL((TMsvId)message_id, unread))
  
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", unread);
}

extern "C" PyObject *
inb_message_type(INB_object* self, PyObject *args)
{
  TInt message_id = 0;
  if (!PyArg_ParseTuple(args, "i", &message_id))
    return NULL;

  TUid msgType;
  TRAPD(error, msgType = self->inbox->GetMessageTypeL((TMsvId)message_id));
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", msgType.iUid);
}

/*
 * Get message content size (in bytes).
 */
extern "C" PyObject *
inb_size(INB_object* self, PyObject *args)
{
  TInt message_id = 0;
  if (!PyArg_ParseTuple(args, "i", &message_id))
    return NULL;

  TInt size = 0;
  TRAPD(error, size = self->inbox->GetMessageSizeL((TMsvId)message_id));
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", size);
}

extern "C" PyObject *
inb_data(INB_object* self, PyObject *args)
{
  TInt msgId = 0;
  TInt pos = 0;
  TInt len = 0;
  if (!PyArg_ParseTuple(args, "iii", &msgId, &pos, &len))
    return NULL;

  HBufC8* buf = HBufC8::NewL(len);
  TPtr8 wBuf = buf->Des();
  TRAPD(error, self->inbox->GetMessageDataL(msgId, pos, wBuf));
  if (error != KErrNone) {
    delete buf;
    return SPyErr_SetFromSymbianOSErr(error);
  }
  PyObject* res = Py_BuildValue("s#", wBuf.Ptr(), wBuf.Length());
  delete buf;
  return res;
}

extern "C" PyObject *
inb_description(INB_object* self, PyObject *args)
{
  TInt msgId = 0;
  if (!PyArg_ParseTuple(args, "i", &msgId))
    return NULL;

  TFileName fileName; // should at least fit
  TRAPD(error, self->inbox->GetDescriptionL((TMsvId)msgId, fileName));
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
  
  return Py_BuildValue("u#", fileName.Ptr(), fileName.Length());
}

extern "C" PyObject *
inb_attachment_path(INB_object* self, PyObject *args)
{
  TInt msgId = 0;
  if (!PyArg_ParseTuple(args, "i", &msgId))
    return NULL;

  TFileName fileName;
  TRAPD(error, self->inbox->GetAttachmentPathL((TMsvId)msgId, fileName));
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
  
  return Py_BuildValue("u#", fileName.Ptr(), fileName.Length());
}

/*
 * Get a list of SMS messages in inbox.
 */
extern "C" PyObject *
inb_list_messages(INB_object* self, PyObject *args)
{
  TInt msgTypeInt;
  if (!PyArg_ParseTuple(args, "i", &msgTypeInt))
    return NULL;
  TUid msgType;
  msgType.iUid = msgTypeInt;

  CArrayFixFlat<TMsvId>* sms_list=NULL;
  TRAPD(error, sms_list = self->inbox->GetMessagesL(msgType))
    
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
  
  PyObject* r;

  r = PyList_New(0);
  if (r) {
    for (int i = (sms_list->Count() - 1); i >= 0; i--) {
        TInt id = static_cast<TInt>(sms_list->At(i));
        PyObject* v = Py_BuildValue("i", id);
        if ((v == NULL) || (PyList_Append(r, v) != 0)) {
          Py_XDECREF(v);
          Py_DECREF(r);
          r = NULL;
          break;
         }
        Py_DECREF(v);
    }
  }

  delete sms_list;
  return r;
}

//////////////TYPE SET////////////////////////

extern "C" {

  static const PyMethodDef inb_methods[] = {
    {"bind", (PyCFunction)inb_bind, METH_VARARGS},
    {"delete", (PyCFunction)inb_delete, METH_VARARGS}, 
    {"address", (PyCFunction)inb_address, METH_VARARGS},
    {"text", (PyCFunction)inb_text, METH_VARARGS},
    {"data", (PyCFunction)inb_data, METH_VARARGS},
    {"size", (PyCFunction)inb_size, METH_VARARGS},
    {"attachment_path", (PyCFunction)inb_attachment_path, METH_VARARGS},
    {"description", (PyCFunction)inb_description, METH_VARARGS},
    {"time", (PyCFunction)inb_time, METH_VARARGS},
    {"unread", (PyCFunction)inb_unread, METH_VARARGS},
    {"list_messages", (PyCFunction)inb_list_messages, METH_VARARGS},
    {"message_type", (PyCFunction)inb_message_type, METH_VARARGS},
    {NULL,              NULL}           /* sentinel */
  };

  static PyObject *
  inb_getattr(INB_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)inb_methods, (PyObject *)op, name);
  }

  static const PyTypeObject c_inb_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "pyinbox.Inbox",                            /*tp_name*/
    sizeof(INB_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)inb_dealloc,                  /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)inb_getattr,                 /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };
} /* extern "C" */

//////////////INIT////////////////////////////

extern "C" {

  static const PyMethodDef inbox_methods[] = {
    {"Inbox", (PyCFunction)new_inb_object, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initpyinbox(void)
  {
    PyTypeObject* inb_type = PyObject_New(PyTypeObject, &PyType_Type);
    *inb_type = c_inb_type;
    inb_type->ob_type = &PyType_Type;

    SPyAddGlobalString("PyINBType", (PyObject*)inb_type);

    PyObject *m, *d;

    m = Py_InitModule("pyinbox", (PyMethodDef*)inbox_methods);
    d = PyModule_GetDict(m);
    
    PyDict_SetItemString(d,"EInbox", PyInt_FromLong(KMsvGlobalInBoxIndexEntryId));
    PyDict_SetItemString(d,"EOutbox", PyInt_FromLong(KMsvGlobalOutBoxIndexEntryId));
    PyDict_SetItemString(d,"ESent", PyInt_FromLong(KMsvSentEntryId)); 
    PyDict_SetItemString(d,"EDraft", PyInt_FromLong(KMsvDraftEntryId));    
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif
