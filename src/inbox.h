/**
 * ====================================================================
 * inbox.h
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

#include <Python.h>

#include "symbian_python_ext_util.h"
#include "local_epoc_py_utils.h"
#include <e32std.h>
#include <eikenv.h>
#include <txtrich.h>

#include <mtclbase.h>
#include <msvapi.h>
#include <mtclreg.h>
#include <msvids.h>
#include <smut.h>

#include "sconfig.hrh"

#ifdef __DO_LOGGING__
#include <flogger.h> // flogger.lib required, Symbian 7.0-up
#endif

const TInt KMessageBodySize = 512;
const TInt KMessageAddressLength = 512;

class TPyInbCallBack
  {
  public:
    TInt NewInboxEntryCreated(TInt aArg);
  public:
    PyObject* iCb; // Not owned.
  };

NONSHARABLE_CLASS(CInboxAdapter) : public CBase, public MMsvSessionObserver
  {
public:
  static CInboxAdapter* NewL(const TUid& aMsgType, TMsvId&);
  static CInboxAdapter* NewLC(const TUid& aMsgType, TMsvId&);
  ~CInboxAdapter();
public: 
  void DeleteMessageL(TMsvId aMessageId);
  void GetMessageTimeL(TMsvId aMessageId, TTime& aTime);
  void GetMessageUnreadL(TMsvId aMessageId, TBool& aUnread);
  void GetMessageAddressL(TMsvId aMessageId, TDes& aAddress);
  CArrayFixFlat<TMsvId>* GetMessagesL(const TUid& aMsgType);  // Passes ownership 
  TBool GetMessageL(TMsvId aMessageId, TDes& aMessage);

  void GetAttachmentPathL(TMsvId aMessageId, TFileName& aFileName);
  void GetDescriptionL(TMsvId aMessageId, TDes& aDesc);
  TInt GetMessageSizeL(TMsvId aMessageId);
  void GetMessageDataL(TMsvId aMessageId, TInt aOffset, TDes8& aData);
  TUid GetMessageTypeL(TMsvId aMessageId);

  void SetCallBack(TPyInbCallBack &aCb);
private:
  void ConstructL();
  void CompleteConstructL();
private:
  TPyInbCallBack iCallMe;
  TInt iErrorState;
  TBool iCallBackSet;
private:          // from MMsvSessionObserver
  void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3);
private:
  CMsvSession* iSession;          // msg server session
  CBaseMtm* iMtm;                 // Client MTM
  CClientMtmRegistry* iMtmReg;    // Client MTM registry
  TMsvId iFolderType;
  TUid iMsgType;
#ifdef __DO_LOGGING__
  RFileLogger* iLogger; // NULL if not available
#endif
  };
  
//////////////TYPE DEFINITION/////////////////

#define INB_type ((PyTypeObject*)SPyGetGlobalString("PyINBType"))

struct INB_object {
  PyObject_VAR_HEAD
  CInboxAdapter* inbox;
  TPyInbCallBack myCallBack;
  TBool callBackSet;
};
