/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_PostMessageEvent_h
#define mozilla_dom_PostMessageEvent_h

#include "mozilla/dom/Event.h"
#include "mozilla/dom/ipc/StructuredCloneData.h"
#include "mozilla/dom/StructuredCloneHolder.h"
#include "nsCOMPtr.h"
#include "mozilla/MaybeOneOf.h"
#include "mozilla/RefPtr.h"
#include "nsContentUtils.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"

class nsGlobalWindowOuter;
class nsGlobalWindowInner;
class nsIPrincipal;

namespace mozilla {
namespace dom {

class BrowsingContext;

/**
 * Class used to represent events generated by calls to Window.postMessage,
 * which asynchronously creates and dispatches events.
 */
class PostMessageEvent final : public Runnable {
 public:
  NS_DECL_NSIRUNNABLE

  // aCallerWindowID should not be 0.
  PostMessageEvent(BrowsingContext* aSource, const nsAString& aCallerOrigin,
                   nsGlobalWindowOuter* aTargetWindow,
                   nsIPrincipal* aProvidedPrincipal, uint64_t aCallerWindowID,
                   nsIURI* aCallerDocumentURI)
      : PostMessageEvent(aSource, aCallerOrigin, aTargetWindow,
                         aProvidedPrincipal, Some(aCallerWindowID),
                         aCallerDocumentURI, false) {}

  // To be used if there is no WindowID for the PostMessage caller's window (for
  // example because it lives in a different process).
  PostMessageEvent(BrowsingContext* aSource, const nsAString& aCallerOrigin,
                   nsGlobalWindowOuter* aTargetWindow,
                   nsIPrincipal* aProvidedPrincipal, nsIURI* aCallerDocumentURI,
                   bool aIsFromPrivateWindow)
      : PostMessageEvent(aSource, aCallerOrigin, aTargetWindow,
                         aProvidedPrincipal, Nothing(), aCallerDocumentURI,
                         aIsFromPrivateWindow) {}

  void Write(JSContext* aCx, JS::Handle<JS::Value> aMessage,
             JS::Handle<JS::Value> aTransfer, ErrorResult& aError) {
    mHolder.construct<StructuredCloneHolder>(
        StructuredCloneHolder::CloningSupported,
        StructuredCloneHolder::TransferringSupported,
        JS::StructuredCloneScope::SameProcessSameThread);
    mHolder.ref<StructuredCloneHolder>().Write(aCx, aMessage, aTransfer,
                                               JS::CloneDataPolicy(), aError);
  }
  void UnpackFrom(const ClonedMessageData& aMessageData) {
    mHolder.construct<ipc::StructuredCloneData>();
    // FIXME Want to steal!
    //       See https://bugzilla.mozilla.org/show_bug.cgi?id=1516349.
    mHolder.ref<ipc::StructuredCloneData>().CopyFromClonedMessageDataForChild(
        aMessageData);
  }

 private:
  PostMessageEvent(BrowsingContext* aSource, const nsAString& aCallerOrigin,
                   nsGlobalWindowOuter* aTargetWindow,
                   nsIPrincipal* aProvidedPrincipal,
                   const Maybe<uint64_t>& aCallerWindowID,
                   nsIURI* aCallerDocumentURI, bool aIsFromPrivateWindow);
  ~PostMessageEvent();

  void Dispatch(nsGlobalWindowInner* aTargetWindow, Event* aEvent);

  void DispatchError(JSContext* aCx, nsGlobalWindowInner* aTargetWindow,
                     mozilla::dom::EventTarget* aEventTarget);

  RefPtr<BrowsingContext> mSource;
  nsString mCallerOrigin;
  RefPtr<nsGlobalWindowOuter> mTargetWindow;
  nsCOMPtr<nsIPrincipal> mProvidedPrincipal;
  // If the postMessage call was made on a WindowProxy whose Window lives in a
  // separate process then mHolder will contain a StructuredCloneData, else
  // it'll contain a StructuredCloneHolder.
  MaybeOneOf<StructuredCloneHolder, ipc::StructuredCloneData> mHolder;
  Maybe<uint64_t> mCallerWindowID;
  nsCOMPtr<nsIURI> mCallerDocumentURI;
  // This is only set to a relevant value if mCallerWindowID doesn't contain a
  // value.
  bool mIsFromPrivateWindow;
};

}  // namespace dom
}  // namespace mozilla

#endif  // mozilla_dom_PostMessageEvent_h