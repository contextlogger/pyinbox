/**
 * ====================================================================
 * inboxadapter.cpp
 * Copyright (c) 2005-2007 Nokia Corporation
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

#ifdef __SYMBIAN_9__
#include <mmsvattachmentmanager.h>
#endif

// A helper function for the implementation of callbacks
// from C/C++ code to Python callables (modified from appuifwmodule.cpp)
TInt TPyInbCallBack::NewInboxEntryCreated(TInt aArg)
  {
  PyEval_RestoreThread(PYTHON_TLS->thread_state);

  TInt error = KErrNone;
  
  PyObject* arg = Py_BuildValue("(i)", aArg);
  PyObject* rval = PyEval_CallObject(iCb, arg);
  
  Py_DECREF(arg);
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      if (PyInt_Check(value))
        error = PyInt_AS_LONG(value);
      Py_XDECREF(type);
      Py_XDECREF(value);
      Py_XDECREF(traceback);
    } else {
      PyErr_Print();
    }
  }
  else
    Py_DECREF(rval);  

  PyEval_SaveThread();
  return error;
  }

//////////////////////////////////////////////

CInboxAdapter* CInboxAdapter::NewL(const TUid& aMsgType, TMsvId& aFolderType)
  {
  CInboxAdapter* self = NewLC(aMsgType, aFolderType);
  CleanupStack::Pop(self); 
  return self;
  }

CInboxAdapter* CInboxAdapter::NewLC(const TUid& aMsgType, TMsvId& aFolderType)
  {
  CInboxAdapter* self = new (ELeave) CInboxAdapter();
  CleanupStack::PushL(self);
  // note how setting these here rather than in ctor; okay since private ctor
  self->iMsgType = aMsgType;
  self->iFolderType = aFolderType;
  self->ConstructL();
  return self;
  }

void CInboxAdapter::ConstructL()
  {
#ifdef __DO_LOGGING__
  iLogger = new (ELeave) RFileLogger;
  if (iLogger->Connect() != KErrNone) {
    delete iLogger;
    iLogger = NULL;
  } else {
    _LIT(KLoggerDir, "pyinbox");
    _LIT(KLoggerFile, "log.txt");
    iLogger->CreateLog(KLoggerDir, KLoggerFile, EFileLoggingModeOverwrite);
    iLogger->Write(_L("logging started"));
  }
#endif

  iCallBackSet = EFalse;
  iSession = CMsvSession::OpenSyncL(*this); // new session is opened synchronously  
  CompleteConstructL();       // Construct the mtm registry
  }

void CInboxAdapter::CompleteConstructL()
  {
  // We get a MtmClientRegistry from our session
  // this registry is used to instantiate new mtms.
  iMtmReg = CClientMtmRegistry::NewL(*iSession);
  //iMtm = iMtmReg->NewMtmL(KUidMsgTypeSMS);        // create new SMS MTM
  iMtm = iMtmReg->NewMtmL(iMsgType);
  }

CInboxAdapter::~CInboxAdapter()
  {
  delete iMtm;
  delete iMtmReg;
  delete iSession; // must be last to be deleted

#ifdef __DO_LOGGING__
  if (iLogger) {
    iLogger->CloseLog();
    iLogger->Close();
    delete iLogger;
  }
#endif
  }

void CInboxAdapter::DeleteMessageL(TMsvId aMessageId)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);
  iMtm->LoadMessageL();

	TMsvId parent = iMtm->Entry().Entry().Parent();

	iMtm->SwitchCurrentEntryL(parent);

	iMtm->Entry().DeleteL(aMessageId);
	}

void CInboxAdapter::GetMessageTimeL(TMsvId aMessageId, TTime& aTime)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);	
	iMtm->LoadMessageL();

  aTime = (iMtm->Entry().Entry().iDate);
	}

void CInboxAdapter::GetMessageUnreadL(TMsvId aMessageId, TBool& aUnread)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);	
	iMtm->LoadMessageL();

	aUnread = (iMtm->Entry().Entry().Unread());
	}

void CInboxAdapter::GetMessageAddressL(TMsvId aMessageId, TDes& aAddress)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);	
	iMtm->LoadMessageL();
  TPtrC address = iMtm->Entry().Entry().iDetails;
	TInt length = address.Length();
	
	// Check length because address is read to a limited size TBuf
	if (length >= KMessageAddressLength) 
	  {
		aAddress.Append(address.Left(KMessageAddressLength - 1));
		}
	else 
		{
		aAddress.Append(address.Left(length));
	  }  
	}  

CArrayFixFlat<TMsvId>* CInboxAdapter::GetMessagesL(const TUid& aMsgType)
  {
  // XXX add here e.g. e-mail and MMS type also:
  // used to be fixed to aMsgType == KUidMsgTypeSMS

    // btmsgtypeuid.h:const TInt32 KUidMsgTypeBtTInt32 = 0x10009ED5;
    // btmsgtypeuid.h:const TUid KUidMsgTypeBt = {KUidMsgTypeBtTInt32};

  CMsvEntry* parentEntry=NULL;
  CMsvEntrySelection* entries=NULL;
  parentEntry = CMsvEntry::NewL(*iSession, 
                                iFolderType, 
                                TMsvSelectionOrdering()); //pointer to Messages Inbox
  entries = parentEntry->ChildrenWithMtmL(aMsgType);
  delete parentEntry;
  return (CArrayFixFlat<TMsvId>*)entries;
  }

void CInboxAdapter::GetDescriptionL(TMsvId aMessageId, TDes& aDesc)
{
  iMtm->SwitchCurrentEntryL(aMessageId);
  iMtm->LoadMessageL();

  CMsvEntry& msvEntry = iMtm->Entry();
  const TMsvEntry& indexEntry = msvEntry.Entry();

  // This should hopefully give us the filename component. Do not
  // know if this applies to all attachments and message types.
  aDesc.Copy(indexEntry.iDescription);
}

// aFileName probably should be of type TFileName, to ensure that any
// legal pathname will fit.
void CInboxAdapter::GetAttachmentPathL(TMsvId aMessageId, TFileName& aFileName)
{
#ifdef __SYMBIAN_9__
  // v9 does not even have the GetFilePath method, so cannot do this
  // even if happened to have enough capabilities to actually access
  // the messaging directory tree.
  User::Leave(KErrNotSupported);
#else
  iMtm->SwitchCurrentEntryL(aMessageId);
  iMtm->LoadMessageL();

  CMsvEntry& msvEntry = iMtm->Entry();
  const TMsvEntry& indexEntry = msvEntry.Entry();

  CMsvEntrySelection* children = msvEntry.ChildrenL();
  CleanupStack::PushL(children);
  TInt childCount = children->Count();
  if (childCount == 1) {
    TMsvId attId = (*children)[0];
    CMsvEntry* attEntry = iSession->GetEntryL(attId);
    CleanupStack::PushL(attEntry);
    // This should get us the path component of the pathname. The TInt
    // return value of this method is undocumented.
    attEntry->GetFilePath(aFileName);
    CleanupStack::PopAndDestroy(attEntry);
    // This should hopefully give us the filename component. Do not
    // know if this applies to all attachments and message types.
    aFileName.Append(indexEntry.iDescription);
  } else {
    // Unless there is exactly one child, we do not know which one to
    // examine.
    User::Leave(KErrNotFound);
  }
  CleanupStack::PopAndDestroy(children);
#endif
}

TInt CInboxAdapter::GetMessageSizeL(TMsvId aMessageId)
{
#ifndef __SYMBIAN_9__
  // We could implement a replacement for the unavailable attachment
  // handler on v8, but cannot be bothered, since such file handling
  // is much easier in Python.
  User::Leave(KErrNotSupported);
  return 0;
#else
  TInt msgSize = 0;

  iMtm->SwitchCurrentEntryL(aMessageId);
  iMtm->LoadMessageL();

  CMsvEntry& msvEntry = iMtm->Entry();
  //const TMsvEntry& indexEntry = msvEntry.Entry();

  CMsvEntrySelection* children = msvEntry.ChildrenL();
  CleanupStack::PushL(children);
  TInt childCount = children->Count();
  if (childCount == 1) {
    TMsvId attId = (*children)[0];
    CMsvEntry* attEntry = iSession->GetEntryL(attId);
    CleanupStack::PushL(attEntry);
    if (attEntry->HasStoreL()) {
      CMsvStore* store = attEntry->ReadStoreL();
      CleanupStack::PushL(store);
      MMsvAttachmentManager& attMan = store->AttachmentManagerL();
      TInt attCount = attMan.AttachmentCount();
      if (attCount == 1) {
	CMsvAttachment* attInfo = attMan.GetAttachmentInfoL(0);
	msgSize = attInfo->Size();
	delete attInfo;
      } else {
        // Unless there is exactly one attachment, we do not know
        // which one to examine.
	User::Leave(KErrNotFound);
      }
      CleanupStack::PopAndDestroy(store);
    } else {
      // No store to examine.
      User::Leave(KErrNotFound);
    }
    CleanupStack::PopAndDestroy(attEntry);
  } else {
    // Unless there is exactly one child, we do not know which one to
    // examine.
    User::Leave(KErrNotFound);
  }
  CleanupStack::PopAndDestroy(children);

  return msgSize;
#endif
}

// Extracts at most aData.MaxLength() bytes of message content,
// starting from offset aOffset. If there is no message data to
// return, then aData will be set to an empty byte string; otherwise
// at least 1 byte will be written to aData. aData may not have a 0
// MaxLength().
void CInboxAdapter::GetMessageDataL(TMsvId aMessageId, TInt aOffset, 
				    TDes8& aData)
{
#ifndef __SYMBIAN_9__
  // We could implement a replacement for the unavailable attachment
  // handler on v8, but cannot be bothered, since such file handling
  // is much easier in Python.
  User::Leave(KErrNotSupported);
  return;
#else
  if (aData.MaxLength() == 0)
    // Okay, since this whole thing is an internal API.
    // Just never call with such a buffer.
    User::Invariant();

  iMtm->SwitchCurrentEntryL(aMessageId);
  iMtm->LoadMessageL();

  CMsvEntry& msvEntry = iMtm->Entry();

  CMsvEntrySelection* children = msvEntry.ChildrenL();
  CleanupStack::PushL(children);
  TInt childCount = children->Count();
  if (childCount == 1) {
    TMsvId attId = (*children)[0];
    CMsvEntry* attEntry = iSession->GetEntryL(attId);
    CleanupStack::PushL(attEntry);
    if (attEntry->HasStoreL()) {
      CMsvStore* store = attEntry->ReadStoreL();
      CleanupStack::PushL(store);
      MMsvAttachmentManager& attMan = store->AttachmentManagerL();
      TInt attCount = attMan.AttachmentCount();
      if (attCount == 1) {
	RFile file = attMan.GetAttachmentFileL(0);
	CleanupClosePushL(file);
	User::LeaveIfError(file.Read(aOffset, aData));
	CleanupStack::PopAndDestroy(); // file
      } else {
        // Unless there is exactly one attachment, we do not know
        // which one to examine.
	User::Leave(KErrNotFound);
      }
      CleanupStack::PopAndDestroy(store);
    } else {
      // No store to examine.
      User::Leave(KErrNotFound);
    }
    CleanupStack::PopAndDestroy(attEntry);
  } else {
    // Unless there is exactly one child, we do not know which one to
    // examine.
    User::Leave(KErrNotFound);
  }
  CleanupStack::PopAndDestroy(children);
#endif
}

TUid CInboxAdapter::GetMessageTypeL(TMsvId aMessageId)
{
  iMtm->SwitchCurrentEntryL(aMessageId);
  return iMtm->Type();
}

TBool CInboxAdapter::GetMessageL(TMsvId aMessageId, TDes& aMessage)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);

  if (iMtm->Entry().HasStoreL()) 
		{
		// SMS message is stored inside Messaging store
		CMsvStore* store = iMtm->Entry().ReadStoreL();
		CleanupStack::PushL(store);
	
		if (store->HasBodyTextL())
			{
			CEikonEnv* const env = CEikonEnv::Static();
			CParaFormatLayer* pfl;
			CCharFormatLayer* cfl;
			
			if (env)
				{
				pfl = env->SystemParaFormatLayerL();
				cfl = env->SystemCharFormatLayerL();
				}
			else
				{
				pfl = CParaFormatLayer::NewL();
				CleanupStack::PushL(pfl);
				cfl = CCharFormatLayer::NewL();
				CleanupStack::PushL(cfl);
				}

			CRichText* richText = CRichText::NewL(pfl, cfl);
			richText->Reset();
			CleanupStack::PushL(richText);

			// Get the SMS body text.
			store->RestoreBodyTextL(*richText);
			const TInt length = richText->DocumentLength();
			TBuf<KMessageBodySize> message;

			// Check length because message is read to limited size TBuf
			if (length >= KMessageBodySize) 
				{
				message.Append(richText->Read(0, KMessageBodySize -1));
				}
			else 
				{
				message.Append(richText->Read(0, length));
				}

			aMessage.Append(message);
			CleanupStack::PopAndDestroy(richText);
			
			if (!env)
				{
				CleanupStack::PopAndDestroy(cfl);
				CleanupStack::PopAndDestroy(pfl);
				}
			}
		
		CleanupStack::PopAndDestroy(store);
		
		}
	else
		{
		return EFalse;
		}

	return ETrue;
	}

void CInboxAdapter::SetCallBack(TPyInbCallBack &aCb) 
  {
  iCallMe = aCb;
  iCallBackSet = ETrue;
  }

void CInboxAdapter::HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* /*aArg3*/)
  {
      TInt i; 
      switch(aEvent) {
	      // New entries only
        case EMsvEntriesCreated:
          {
          if (iCallBackSet) {
            // Messages that are created in Inbox
            TMsvId* parent;
	          parent = static_cast<TMsvId*>(aArg2);

            // XXX is this needed for Outbox etc.?
            // Check the parent folder to be global inbox
	          if(*parent != KMsvGlobalInBoxIndexEntryId) 
	            return;

	          CMsvEntrySelection* entries = static_cast<CMsvEntrySelection*>(aArg1);
	          for(i = 0; i < entries->Count(); i++) {
	            iErrorState = iCallMe.NewInboxEntryCreated(static_cast<TInt>(entries->At(i)));
	          }
          }
	        break;
          }
        default:
	        break;
     }
  }
