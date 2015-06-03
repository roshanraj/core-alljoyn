/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "PermissionMgmtTest.h"
#include "KeyInfoHelper.h"
#include <qcc/Crypto.h>
#include <string>

using namespace ajn;
using namespace qcc;

static GUID128 membershipGUID1(1);
static const char* membershipSerial0 = "10000";
static const char* membershipSerial1 = "10001";
static GUID128 membershipGUID2(2);
static GUID128 membershipGUID3(3);
static GUID128 membershipGUID4(4);
static const char* membershipSerial2 = "20002";
static const char* membershipSerial3 = "30003";
static const char* membershipSerial4 = "40004";

static const char sampleCertificatePEM[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "AAAAAf8thIwHzhCU8qsedyuEldP/TouX6w7rZI/cJYST/kexAAAAAMvbuy8JDCJI\n"
    "Ms8vwkglUrf/infSYMNRYP/gsFvl5FutAAAAAAAAAAD/LYSMB84QlPKrHncrhJXT\n"
    "/06Ll+sO62SP3CWEk/5HsQAAAADL27svCQwiSDLPL8JIJVK3/4p30mDDUWD/4LBb\n"
    "5eRbrQAAAAAAAAAAAAAAAAASgF0AAAAAABKBiQABMa7uTLSqjDggO0t6TAgsxKNt\n"
    "+Zhu/jc3s242BE0drFU12USXXIYQdqps/HrMtqw6q9hrZtaGJS+e9y7mJegAAAAA\n"
    "APpeLT1cHNm3/OupnEcUCmg+jqi4SUEi4WTWSR4OzvCSAAAAAA==\n"
    "-----END CERTIFICATE-----"
};

static void GenerateAdminGroupACL(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, PermissionPolicy::Acl& acl)
{
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(groupGUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(groupAuthority.GetKeyId(), groupAuthority.GetKeyIdLen());
    keyInfo->SetPublicKey(groupAuthority.GetPublicKey());
    peers[0].SetKeyInfo(keyInfo);
    acl.SetPeers(1, peers);

    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(
        PermissionPolicy::Rule::Member::ACTION_PROVIDE |
        PermissionPolicy::Rule::Member::ACTION_OBSERVE |
        PermissionPolicy::Rule::Member::ACTION_MODIFY
        );
    rules[0].SetMembers(1, prms);
    acl.SetRules(1, rules);
}

static PermissionPolicy* GenerateWildCardPolicy(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(52516);

    /* add the acls section */
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[2];
    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("org.allseenalliance.control.*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(3, prms);
    acls[0].SetRules(1, rules);

    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[1]);
    policy->SetAcls(2, acls);

    return policy;
}

static PermissionPolicy* GeneratePolicy(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, BusAttachment& guildAuthorityBus)
{
    qcc::GUID128 guildAuthorityGUID;
    PermissionMgmtTestHelper::GetGUID(guildAuthorityBus, guildAuthorityGUID);
    ECCPublicKey guildAuthorityPubKey;
    QStatus status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(guildAuthorityBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);

    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(74892317);

    /* add the acls section */
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[5];
    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[3];
    rules[0].SetObjPath("/control/guide");
    rules[0].SetInterfaceName("allseenalliance.control.*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(0);  /* action denied */
    prms[1].SetMemberName("*");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(2, prms);
    rules[2].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("ChannelChanged");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[2].SetMembers(1, prms);
    acls[0].SetRules(3, rules);

    /* acls record 1 GUILD membershipGUID1 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(membershipGUID1);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guildAuthorityGUID.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthorityPubKey);
    peers[0].SetKeyInfo(keyInfo);
    acls[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[5];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("Volume");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[3].SetMemberName("InputSource");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[4].SetMemberName("Caption");
    prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(5, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    acls[1].SetRules(2, rules);

    /* acls record 2 GUILD membershipGUID2 */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(membershipGUID2);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guildAuthorityGUID.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthorityPubKey);
    peers[0].SetKeyInfo(keyInfo);
    acls[2].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[3];
    rules[0].SetObjPath("/control/settings");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(0);  /* action denied */
    rules[0].SetMembers(1, prms);
    rules[1].SetObjPath("/control/guide");
    rules[1].SetInterfaceName("org.allseenalliance.control.*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    rules[2].SetObjPath("*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[2].SetMembers(1, prms);
    acls[2].SetRules(3, rules);

    /* acls record 3 peer specific rule  */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
    keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guildAuthorityGUID.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthorityPubKey);
    peers[0].SetKeyInfo(keyInfo);
    acls[3].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("Mute");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    acls[3].SetRules(1, rules);

    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[4]);
    policy->SetAcls(5, acls);

    return policy;
}

static PermissionPolicy* GeneratePolicyForSpecificCA(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, const KeyInfoNISTP256& restrictedCA)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(25531);

    /* add the acls section */
    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[3];
    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[3];
    rules[0].SetObjPath("/control/guide");
    rules[0].SetInterfaceName("allseenalliance.control.*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(1, prms);
    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(0);  /* action denied */
    prms[1].SetMemberName("*");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(2, prms);
    rules[2].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("ChannelChanged");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[2].SetMembers(1, prms);
    acls[0].SetRules(3, rules);

    /* acls record 1 FROM_CERTIFICATE_AUTHORITY */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256(restrictedCA);
    peers[0].SetKeyInfo(keyInfo);
    acls[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[5];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("Volume");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[3].SetMemberName("InputSource");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[4].SetMemberName("Caption");
    prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(5, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);
    acls[1].SetRules(2, rules);

    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[2]);
    policy->SetAcls(3, acls);

    return policy;
}

static PermissionPolicy* GenerateSmallAnyUserPolicy(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(55);

    /* add the acls ection */

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[2];

    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("On");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);
    rules[1].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("ChannelChanged");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[1].SetMembers(1, prms);
    acls[0].SetRules(2, rules);

    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[1]);
    policy->SetAcls(2, acls);

    return policy;
}

static PermissionPolicy* GenerateFullAccessAnyUserPolicy(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, bool allowSignal)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(552317);

    /* add the acls ection */

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[2];

    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Off");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("On");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);
    rules[1].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    size_t count = 0;
    if (allowSignal) {
        prms = new PermissionPolicy::Rule::Member[2];
        prms[count].SetMemberName("ChannelChanged");
        prms[count].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
        prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
        count++;
    } else {
        prms = new PermissionPolicy::Rule::Member[1];
    }
    prms[count].SetMemberName("*");
    prms[count].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(count + 1, prms);

    acls[0].SetRules(2, rules);

    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[1]);
    policy->SetAcls(2, acls);
    return policy;
}

static PermissionPolicy* GenerateAnyUserDeniedPrefixPolicy(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(552317);

    /* add the incoming section */

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[2];

    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Of*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(0);  /* action denied */
    prms[1].SetMemberName("*");
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);
    rules[1].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("ChannelChanged");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[1].SetMembers(1, prms);
    acls[0].SetRules(2, rules);


    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[1]);
    policy->SetAcls(2, acls);
    return policy;
}

static PermissionPolicy* GenerateFullAccessOutgoingPolicy(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, bool allowIncomingSignal)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(38276);

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[2];

    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member* prms = NULL;
    size_t count = 0;
    if (allowIncomingSignal) {
        prms = new PermissionPolicy::Rule::Member[3];
        prms[count].SetMemberName("*");
        prms[count].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
        prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        count++;
    } else {
        prms = new PermissionPolicy::Rule::Member[2];
    }
    prms[count].SetMemberName("*");
    prms[count].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    count++;
    prms[count].SetMemberName("*");
    prms[count].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    count++;
    rules[0].SetMembers(count, prms);
    acls[0].SetRules(1, rules);

    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[1]);
    policy->SetAcls(2, acls);
    return policy;
}

static PermissionPolicy* GenerateFullAccessOutgoingPolicy(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority)
{
    return GenerateFullAccessOutgoingPolicy(groupGUID, groupAuthority, true);
}

static PermissionPolicy* GenerateFullAccessOutgoingPolicyWithGuestServices(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, bool allowIncomingSignal, const KeyInfoNISTP256& guestCA)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(2737834);

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[3];

    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member* prms = NULL;
    size_t count = 0;
    if (allowIncomingSignal) {
        prms = new PermissionPolicy::Rule::Member[3];
        prms[count].SetMemberName("*");
        prms[count].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
        prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
        count++;
    } else {
        prms = new PermissionPolicy::Rule::Member[2];
    }
    prms[count].SetMemberName("*");
    prms[count].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    count++;
    prms[count].SetMemberName("*");
    prms[count].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[count].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    count++;
    rules[0].SetMembers(count, prms);
    acls[0].SetRules(1, rules);

    /* acls record 1 FROM_CERTIFICATE_AUTHORITY */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_FROM_CERTIFICATE_AUTHORITY);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256(guestCA);
    peers[0].SetKeyInfo(keyInfo);
    acls[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName("*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    rules[0].SetMembers(1, prms);
    acls[1].SetRules(1, rules);

    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[2]);
    policy->SetAcls(3, acls);
    return policy;
}

static PermissionPolicy* GenerateGuildSpecificAccessOutgoingPolicy(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, const GUID128& guildGUID, BusAttachment& guildAuthorityBus)
{
    qcc::GUID128 guildAuthorityGUID;
    PermissionMgmtTestHelper::GetGUID(guildAuthorityBus, guildAuthorityGUID);
    ECCPublicKey guildAuthorityPubKey;
    QStatus status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(guildAuthorityBus, &guildAuthorityPubKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);

    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(827425);

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[3];

    /* acls record 0  ANY-USER */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_ANY_TRUSTED);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    rules[0].SetMembers(3, prms);
    acls[0].SetRules(1, rules);

    /* acls record 1 GUILD specific */
    peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_MEMBERSHIP);
    peers[0].SetSecurityGroupId(guildGUID);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetKeyId(guildAuthorityGUID.GetBytes(), qcc::GUID128::SIZE);
    keyInfo->SetPublicKey(&guildAuthorityPubKey);
    peers[0].SetKeyInfo(keyInfo);
    acls[1].SetPeers(1, peers);
    rules = new PermissionPolicy::Rule[1];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[3];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[1].SetMemberName("*");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE);
    prms[2].SetMemberName("*");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_OBSERVE);

    rules[0].SetMembers(3, prms);
    acls[1].SetRules(1, rules);

    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[2]);
    policy->SetAcls(3, acls);
    return policy;
}

static PermissionPolicy* GeneratePolicyPeerPublicKey(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, qcc::ECCPublicKey& peerPublicKey)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(8742198);

    /* add the provider section */

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[2];

    /* acls record 0 peer */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetPublicKey(&peerPublicKey);
    peers[0].SetKeyInfo(keyInfo);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[4];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("Volume");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[3].SetMemberName("Caption");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(4, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    acls[0].SetRules(2, rules);
    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[1]);
    policy->SetAcls(2, acls);

    return policy;
}

static PermissionPolicy* GeneratePolicyDenyPeerPublicKey(const GUID128& groupGUID, const KeyInfoNISTP256& groupAuthority, qcc::ECCPublicKey& peerPublicKey)
{
    PermissionPolicy* policy = new PermissionPolicy();

    policy->SetSerialNum(32445);

    /* add the provider section */

    PermissionPolicy::Acl* acls = new PermissionPolicy::Acl[2];

    /* acls record 0 peer */
    PermissionPolicy::Peer* peers = new PermissionPolicy::Peer[1];
    peers[0].SetType(PermissionPolicy::Peer::PEER_WITH_PUBLIC_KEY);
    KeyInfoNISTP256* keyInfo = new KeyInfoNISTP256();
    keyInfo->SetPublicKey(&peerPublicKey);
    peers[0].SetKeyInfo(keyInfo);
    acls[0].SetPeers(1, peers);
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[2];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[4];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(0);  /* action denied */
    prms[2].SetMemberName("Volume");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[3].SetMemberName("Caption");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(4, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    acls[0].SetRules(2, rules);
    GenerateAdminGroupACL(groupGUID, groupAuthority, acls[1]);
    policy->SetAcls(2, acls);

    return policy;
}

static QStatus GenerateAllowAllManifest(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 1;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName("*");
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_PROVIDE | PermissionPolicy::Rule::Member::ACTION_MODIFY | PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    rules[0].SetMembers(1, prms);
    *retRules = rules;
    return ER_OK;
}

static QStatus GenerateManifestNoInputSource(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[4];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("ChannelChanged");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    prms[3].SetMemberName("Volume");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(4, prms);

    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
    return ER_OK;
}

static QStatus GenerateManifestDenied(bool denyTVUp, bool denyCaption, PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[5];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    if (denyTVUp) {
        prms[0].SetActionMask(0);  /* action denied */
    } else {
        prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    }
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[2].SetMemberName("ChannelChanged");
    prms[2].SetMemberType(PermissionPolicy::Rule::Member::SIGNAL);
    prms[2].SetActionMask(PermissionPolicy::Rule::Member::ACTION_OBSERVE);
    prms[3].SetMemberName("Volume");
    prms[3].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    prms[3].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[4].SetMemberName("Caption");
    prms[4].SetMemberType(PermissionPolicy::Rule::Member::PROPERTY);
    if (denyCaption) {
        prms[4].SetActionMask(0); /* action denied */
    } else {
        prms[4].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    }
    rules[0].SetMembers(5, prms);

    rules[1].SetInterfaceName(BasePermissionMgmtTest::ONOFF_IFC_NAME);
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
    return ER_OK;
}
static QStatus GenerateManifestTemplate(PermissionPolicy::Rule** retRules, size_t* count)
{
    *count = 2;
    PermissionPolicy::Rule* rules = new PermissionPolicy::Rule[*count];
    rules[0].SetInterfaceName(BasePermissionMgmtTest::TV_IFC_NAME);
    PermissionPolicy::Rule::Member* prms = new PermissionPolicy::Rule::Member[2];
    prms[0].SetMemberName("Up");
    prms[0].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    prms[1].SetMemberName("Down");
    prms[1].SetMemberType(PermissionPolicy::Rule::Member::METHOD_CALL);
    prms[1].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[0].SetMembers(2, prms);

    rules[1].SetInterfaceName("org.allseenalliance.control.Mouse*");
    prms = new PermissionPolicy::Rule::Member[1];
    prms[0].SetMemberName("*");
    prms[0].SetActionMask(PermissionPolicy::Rule::Member::ACTION_MODIFY);
    rules[1].SetMembers(1, prms);

    *retRules = rules;
    return ER_OK;
}

class PermissionMgmtUseCaseTest : public BasePermissionMgmtTest {

  public:
    PermissionMgmtUseCaseTest() : BasePermissionMgmtTest("/app")
    {
    }
    PermissionMgmtUseCaseTest(const char* path) : BasePermissionMgmtTest(path)
    {
    }

    QStatus InvokeClaim(bool useAdminCA, BusAttachment& claimerBus, BusAttachment& claimedBus, qcc::String serial, qcc::String alias, bool expectClaimToFail, BusAttachment* caBus)
    {
        SecurityApplicationProxy saProxy(claimerBus, claimedBus.GetUniqueName().c_str());
        /** TODO: remove pmProxy once the required information is refactored
         * into the SecurityApplicationProxy */
        PermissionMgmtProxy pmProxy(claimerBus, claimedBus.GetUniqueName().c_str());
        /* retrieve public key from to-be-claimed app to create identity cert */
        ECCPublicKey claimedPubKey;
        EXPECT_EQ(ER_OK, pmProxy.GetPublicKey(&claimedPubKey)) << " Fail to retrieve to-be-claimed public key.";
        qcc::GUID128 guid;
        PermissionMgmtTestHelper::GetGUID(claimerBus, guid);
        IdentityCertificate identityCertChain[2];
        size_t certChainCount = 2;
        PermissionPolicy::Rule* manifest = NULL;
        size_t manifestSize = 0;
        GenerateAllowAllManifest(&manifest, &manifestSize);
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(claimerBus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
        if (caBus != NULL) {
            status = PermissionMgmtTestHelper::CreateIdentityCertChain(*caBus, claimerBus, serial, guid.ToString(), &claimedPubKey, alias, 3600, identityCertChain, certChainCount, digest, Crypto_SHA256::DIGEST_SIZE);
        } else {
            certChainCount = 1;
            status = PermissionMgmtTestHelper::CreateIdentityCert(claimerBus, serial, guid.ToString(), &claimedPubKey, alias, 3600, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE);
        }
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.  Actual Status: " << QCC_StatusText(status);

        QStatus status;
        if (useAdminCA) {
            status = saProxy.Claim(adminAdminGroupAuthority, adminAdminGroupGUID, adminAdminGroupAuthority, identityCertChain, certChainCount, manifest, manifestSize);
        } else {
            status = saProxy.Claim(consumerAdminGroupAuthority, consumerAdminGroupGUID, consumerAdminGroupAuthority, identityCertChain, certChainCount, manifest, manifestSize);
        }
        delete [] manifest;
        if (expectClaimToFail) {
            return status;
        }
        EXPECT_EQ(ER_OK, status) << "Claim failed.";

        /* retrieve back the identity cert to compare */
        IdentityCertificate* newCertChain = NULL;
        size_t newCertChainCount = 0;
        status = pmProxy.GetIdentity(&newCertChain, &newCertChainCount);
        EXPECT_EQ(ER_OK, status) << "GetIdentity failed.";
        if ((ER_OK != status) || (newCertChainCount == 0)) {
            return ER_FAIL;
        }
        EXPECT_EQ(newCertChainCount, certChainCount) << "GetIdentity returns back a cert chain in different length than the original.";
        for (size_t cnt = 0; cnt < newCertChainCount; cnt++) {
            qcc::String retIdentity;
            status = newCertChain[cnt].EncodeCertificateDER(retIdentity);
            EXPECT_EQ(ER_OK, status) << "  newCert.EncodeCertificateDER failed.  Actual Status: " << QCC_StatusText(status);
            qcc::String originalIdentity;
            EXPECT_EQ(ER_OK, identityCertChain[cnt].EncodeCertificateDER(originalIdentity)) << "  original cert EncodeCertificateDER failed.";
            EXPECT_EQ(0, memcmp(originalIdentity.data(), retIdentity.data(), originalIdentity.length())) << "  GetIdentity failed.  Return value does not equal original";
        }
        delete [] newCertChain;
        return ER_OK;
    }

    QStatus InvokeClaim(bool useAdminCA, BusAttachment& claimerBus, BusAttachment& claimedBus, qcc::String serial, qcc::String alias, bool expectClaimToFail)
    {
        return InvokeClaim(useAdminCA, claimerBus, claimedBus, serial, alias, expectClaimToFail, NULL);
    }

    /**
     *  Claim the admin app
     */
    void ClaimAdmin()
    {
        QStatus status = ER_OK;

        /* factory reset */
        PermissionConfigurator& pc = adminBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        /* Test Gen DSA keys */
        status = pc.GenerateSigningKeyPair();
        EXPECT_EQ(ER_OK, status) << "  GenerateSigningKeyPair failed.  Actual Status: " << QCC_StatusText(status);
        GenerateCAKeys();

        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminProxyBus, adminBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);

        EXPECT_EQ(ER_OK, InvokeClaim(true, adminProxyBus, adminBus, "1010101", "Admin User", false)) << " InvokeClaim failed.";

        /* reload the shared key store because of change on one bus */
        adminProxyBus.ReloadKeyStore();
        adminBus.ReloadKeyStore();
        qcc::String currentAuthMechanisms = GetAuthMechanisms();
        EnableSecurity("ALLJOYN_ECDHE_ECDSA");
        InstallMembershipToAdmin(adminAdminGroupGUID);
        /* restore the current security mode */
        EnableSecurity(currentAuthMechanisms.c_str());
    }

    /**
     *  Claim the service app
     */
    void ClaimService()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminBus, serviceBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);

        PermissionMgmtProxy pmProxy(adminBus, serviceBus.GetUniqueName().c_str(), sessionId);
        ECCPublicKey claimedPubKey;
        EXPECT_EQ(ER_OK, pmProxy.GetPublicKey(&claimedPubKey)) << "GetPeerPublicKey failed.";

        SetApplicationStateSignalReceived(false);

        /* setup state unclaimable */
        PermissionConfigurator::ApplicationState applicationState;
        EXPECT_EQ(ER_OK, pc.GetApplicationState(applicationState));
        EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationState) << "  ApplicationState is not CLAIMABLE";
        applicationState = PermissionConfigurator::NOT_CLAIMABLE;
        EXPECT_EQ(ER_OK, pc.SetApplicationState(applicationState)) << "  SetApplicationState failed.";
        EXPECT_EQ(ER_OK, pc.GetApplicationState(applicationState));
        EXPECT_EQ(PermissionConfigurator::NOT_CLAIMABLE, applicationState) << "  ApplicationState is not UNCLAIMABLE";

        /* try claiming with state unclaimable.  Exptect to fail */
        EXPECT_EQ(ER_PERMISSION_DENIED, InvokeClaim(true, adminBus, serviceBus, "2020202", "Service Provider", true)) << " InvokeClaim is not supposed to succeed.";

        /* now switch it back to claimable */
        applicationState = PermissionConfigurator::CLAIMABLE;
        EXPECT_EQ(ER_OK, pc.SetApplicationState(applicationState)) << "  SetApplicationState failed.";
        EXPECT_EQ(ER_OK, pc.GetApplicationState(applicationState));

        EXPECT_EQ(PermissionConfigurator::CLAIMABLE, applicationState) << "  ApplicationState is not CLAIMABLE";

        /* try claiming with state claimable.  Expect to succeed */
        EXPECT_EQ(ER_OK, InvokeClaim(true, adminBus, serviceBus, "2020202", "Service Provider", false)) << " InvokeClaim failed.";

        /* try to claim one more time */
        EXPECT_EQ(ER_PERMISSION_DENIED, InvokeClaim(true, adminBus, serviceBus, "2020202", "Service Provider", true)) << " InvokeClaim is not supposed to succeed.";

        ECCPublicKey claimedPubKey2;
        /* retrieve public key from claimed app to validate that it is not changed */
        EXPECT_EQ(ER_OK, pmProxy.GetPublicKey(&claimedPubKey2)) << "GetPeerPublicKey failed.";
        EXPECT_EQ(memcmp(&claimedPubKey2, &claimedPubKey, sizeof(ECCPublicKey)), 0) << "  The public key of the claimed app has changed.";

        /* sleep a second to see whether the ApplicationState signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetApplicationStateSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetApplicationStateSignalReceived()) << " Fail to receive expected ApplicationState signal.";

    }

    /**
     *  Claim the consumer
     */
    void ClaimConsumer()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(adminBus, consumerBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);

        SetApplicationStateSignalReceived(false);
        EXPECT_EQ(ER_OK, InvokeClaim(true, adminBus, consumerBus, "3030303", "Consumer", false)) << " InvokeClaim failed.";

        /* try to claim a second time */
        EXPECT_EQ(ER_PERMISSION_DENIED, InvokeClaim(true, adminBus, consumerBus, "3030303", "Consumer", true)) << "Claim is not supposed to succeed.";

        /* sleep a second to see whether the ApplicationState signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetApplicationStateSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetApplicationStateSignalReceived()) << " Fail to receive expected ApplicationState signal.";
        /* add the consumer admin security group membership cert to consumer */
        qcc::String currentAuthMechanisms = GetAuthMechanisms();
        EnableSecurity("ALLJOYN_ECDHE_ECDSA");
        InstallMembershipToConsumer("100000001", consumerAdminGroupGUID, consumerBus);
        EnableSecurity(currentAuthMechanisms.c_str());
    }

    /**
     *  Claim the remote control by the consumer
     */
    void ConsumerClaimsRemoteControl()
    {
        QStatus status = ER_OK;
        /* factory reset */
        PermissionConfigurator& pc = remoteControlBus.GetPermissionConfigurator();
        status = pc.Reset();
        EXPECT_EQ(ER_OK, status) << "  Reset failed.  Actual Status: " << QCC_StatusText(status);
        SessionId sessionId;
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
        status = PermissionMgmtTestHelper::JoinPeerSession(consumerBus, remoteControlBus, sessionId);
        EXPECT_EQ(ER_OK, status) << "  JoinSession failed.  Actual Status: " << QCC_StatusText(status);
        SetApplicationStateSignalReceived(false);
        EXPECT_EQ(ER_OK, InvokeClaim(false, consumerBus, remoteControlBus, "6060606", "remote control", false, &consumerBus)) << " InvokeClaim failed.";

        /* sleep a second to see whether the ApplicationState signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetApplicationStateSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetApplicationStateSignalReceived()) << " Fail to receive expected ApplicationState signal.";
    }

    void Claims(bool usePSK, bool claimRemoteControl)
    {
        if (usePSK) {
            EnableSecurity("ALLJOYN_ECDHE_PSK");
        } else {
            EnableSecurity("ALLJOYN_ECDHE_NULL");
        }
        ClaimAdmin();
        ClaimService();
        ClaimConsumer();
        if (claimRemoteControl) {
            ConsumerClaimsRemoteControl();
        }
        if (usePSK) {
            EnableSecurity("ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA");
        } else {
            EnableSecurity("ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA");
        }
    }

    void Claims(bool usePSK)
    {
        /* also claims the remote control */
        Claims(usePSK, true);
    }

    /**
     *  Install policy to service app
     */
    void InstallPolicyToAdmin(PermissionPolicy& policy)
    {
        PermissionMgmtProxy pmProxy(adminProxyBus, adminBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, pmProxy.InstallPolicy(policy)) << "  InstallPolicy failed.";

        /* retrieve back the policy to compare */
        PermissionPolicy retPolicy;
        EXPECT_EQ(ER_OK, pmProxy.GetPolicy(&retPolicy)) << "GetPolicy failed.";

        EXPECT_EQ(policy.GetSerialNum(), retPolicy.GetSerialNum()) << " GetPolicy failed. Different serial number.";
        EXPECT_EQ(policy.GetAclsSize(), retPolicy.GetAclsSize()) << " GetPolicy failed. Different incoming acls size.";
    }

    /**
     *  Install policy to app
     */
    void InstallPolicyToNoAdmin(BusAttachment& installerBus, BusAttachment& bus, PermissionPolicy& policy)
    {
        PermissionMgmtProxy pmProxy(installerBus, bus.GetUniqueName().c_str());

        PermissionPolicy aPolicy;
        SetApplicationStateSignalReceived(false);
        EXPECT_EQ(ER_OK, pmProxy.InstallPolicy(policy)) << "InstallPolicy failed.";

        /* retrieve back the policy to compare */
        PermissionPolicy retPolicy;
        EXPECT_EQ(ER_OK, pmProxy.GetPolicy(&retPolicy)) << "GetPolicy failed.";

        EXPECT_EQ(policy.GetSerialNum(), retPolicy.GetSerialNum()) << " GetPolicy failed. Different serial number.";
        EXPECT_EQ(policy.GetAclsSize(), retPolicy.GetAclsSize()) << " GetPolicy failed. Different incoming acls size.";
        /* sleep a second to see whether the ApplicationState signal is received */
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetApplicationStateSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetApplicationStateSignalReceived()) << " Fail to receive expected ApplicationState signal.";
        /* install a policy with the same serial number.  Expect to fail. */
        EXPECT_NE(ER_OK, pmProxy.InstallPolicy(policy)) << "InstallPolicy again with same serial number expected to fail, but it did not.";
    }

    /**
     *  Install policy to service app
     */
    void InstallPolicyToService(PermissionPolicy& policy)
    {
        InstallPolicyToNoAdmin(adminBus, serviceBus, policy);
    }

    /**
     *  Install policy to app
     */
    void InstallPolicyToClientBus(BusAttachment& installerBus, BusAttachment& targetBus, PermissionPolicy& policy)
    {
        InstallPolicyToNoAdmin(installerBus, targetBus, policy);
    }

    /**
     *  Install policy to app
     */
    void InstallPolicyToConsumer(PermissionPolicy& policy)
    {
        InstallPolicyToNoAdmin(adminBus, consumerBus, policy);
    }

    /*
     *  Replace service app Identity Certificate
     */
    void ReplaceIdentityCert(BusAttachment& bus, BusAttachment& targetBus, const PermissionPolicy::Rule* manifest, size_t manifestSize, bool generateRandomSubjectKey)
    {
        PermissionMgmtProxy pmProxy(bus, targetBus.GetUniqueName().c_str());

        /* retrieve the current identity cert */
        IdentityCertificate* certs = NULL;
        size_t count = 0;
        EXPECT_EQ(ER_OK, pmProxy.GetIdentity(&certs, &count)) << "GetIdentity failed.";
        ASSERT_GT(count, (size_t) 0) << "No identity cert found.";

        /* create a new identity cert */
        qcc::String subject((const char*) certs[0].GetSubjectCN(), certs[0].GetSubjectCNLength());
        const qcc::ECCPublicKey* subjectPublicKey;
        if (generateRandomSubjectKey) {
            Crypto_ECC ecc;
            ecc.GenerateDSAKeyPair();
            subjectPublicKey = ecc.GetDSAPublicKey();
        } else {
            subjectPublicKey = certs[0].GetSubjectPublicKey();
        }
        IdentityCertificate identityCertChain[1];
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(bus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
        status = PermissionMgmtTestHelper::CreateIdentityCert(bus, "4040404", subject, subjectPublicKey, "Service Provider", 3600, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.";

        status = pmProxy.InstallIdentity(identityCertChain, 1, manifest, manifestSize);
        if (generateRandomSubjectKey) {
            EXPECT_NE(ER_OK, status) << "InstallIdentity did not fail.";
        } else {
            EXPECT_EQ(ER_OK, status) << "InstallIdentity failed.";
        }
        delete [] certs;
    }

    /*
     *  Replace service app Identity Certificate
     */
    void ReplaceIdentityCert(BusAttachment& bus, BusAttachment& targetBus, bool generateRandomSubjectKey)
    {
        PermissionPolicy::Rule* manifest = NULL;
        size_t manifestSize = 0;
        GenerateAllowAllManifest(&manifest, &manifestSize);
        ReplaceIdentityCert(bus, targetBus, manifest, manifestSize, generateRandomSubjectKey);
        delete [] manifest;
    }

    void ReplaceIdentityCert(BusAttachment& bus, BusAttachment& targetBus)
    {
        ReplaceIdentityCert(bus, targetBus, false);
    }

    void ReplaceIdentityCertWithBadPublicKey(BusAttachment& bus, BusAttachment& targetBus)
    {
        ReplaceIdentityCert(bus, targetBus, true);
    }

    void ReplaceIdentityCertWithExpiredCert(BusAttachment& bus, BusAttachment& targetBus)
    {
        PermissionMgmtProxy pmProxy(bus, targetBus.GetUniqueName().c_str());
        /* retrieve the current identity cert */
        IdentityCertificate* certs = NULL;
        size_t count = 0;
        EXPECT_EQ(ER_OK, pmProxy.GetIdentity(&certs, &count)) << "GetIdentity failed.";
        ASSERT_GT(count, (size_t) 0) << "No identity cert found.";

        /* create a new identity cert */
        qcc::String subject((const char*) certs[0].GetSubjectCN(), certs[0].GetSubjectCNLength());
        IdentityCertificate identityCertChain[1];
        PermissionPolicy::Rule* manifest = NULL;
        size_t manifestSize = 0;
        GenerateAllowAllManifest(&manifest, &manifestSize);
        uint8_t digest[Crypto_SHA256::DIGEST_SIZE];
        EXPECT_EQ(ER_OK, PermissionMgmtObj::GenerateManifestDigest(bus, manifest, manifestSize, digest, Crypto_SHA256::DIGEST_SIZE)) << " GenerateManifestDigest failed.";
        /* create a cert that expires in 1 second */
        status = PermissionMgmtTestHelper::CreateIdentityCert(bus, "4040404", subject, certs[0].GetSubjectPublicKey(), "Service Provider", 1, identityCertChain[0], digest, Crypto_SHA256::DIGEST_SIZE);
        EXPECT_EQ(ER_OK, status) << "  CreateIdentityCert failed.";

        /* sleep 2 seconds to get the cert to expire */
        qcc::Sleep(2000);
        EXPECT_NE(ER_OK, pmProxy.InstallIdentity(identityCertChain, 1, manifest, manifestSize)) << "InstallIdentity did not fail.";
        delete [] certs;
        delete [] manifest;
    }

    /**
     *  Install a membership certificate with the given serial number and guild ID to the service bus attachment.
     */
    void InstallMembershipToServiceProvider(const char* serial, qcc::GUID128& guildID)
    {
        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(serviceBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembership RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN((const char*) serviceGUID.GetBytes(), serviceGUID.SIZE);
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, serviceBus.GetUniqueName(), adminBus, subjectCN, &claimedPubKey, guildID);
        EXPECT_EQ(ER_OK, status) << "  InstallMembership cert1 failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, serviceBus.GetUniqueName(), adminBus, subjectCN, &claimedPubKey, guildID);
        EXPECT_NE(ER_OK, status) << "  InstallMembership cert1 again is supposed to fail.  Actual Status: " << QCC_StatusText(status);
    }

    void InstallMembershipToServiceProvider()
    {
        InstallMembershipToServiceProvider(membershipSerial3, membershipGUID3);
    }

    /**
     *  RemoveMembership from service provider
     */
    void RemoveMembershipFromServiceProvider()
    {
        PermissionMgmtProxy pmProxy(adminBus, serviceBus.GetUniqueName().c_str());
        KeyInfoNISTP256 keyInfo;
        adminBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
        String aki;
        CertificateX509::GenerateAuthorityKeyId(keyInfo.GetPublicKey(), aki);
        EXPECT_EQ(ER_OK, pmProxy.RemoveMembership(membershipSerial3, aki)) << "RemoveMembershipFromServiceProvider failed.";
        /* removing it again */
        EXPECT_NE(ER_OK, pmProxy.RemoveMembership(membershipSerial3, aki)) << "RemoveMembershipFromServiceProvider succeeded.  Expect it to fail.";

    }

    /**
     *  Install Membership to a consumer
     */
    void InstallMembershipToConsumer(const char* serial, qcc::GUID128& guildID, BusAttachment& authorityBus)
    {
        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN((const char*) consumerGUID.GetBytes(), consumerGUID.SIZE);
        status = PermissionMgmtTestHelper::InstallMembership(serial, adminBus, consumerBus.GetUniqueName(), authorityBus, subjectCN, &claimedPubKey, guildID);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToConsumer cert1 failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install Membership to a consumer
     */
    void InstallMembershipToConsumer()
    {
        InstallMembershipToConsumer(membershipSerial1, membershipGUID1, adminBus);
    }

    /**
     *  Install Membership chain to a consumer
     */
    void InstallMembershipChainToTarget(BusAttachment& topBus, BusAttachment& middleBus, BusAttachment& targetBus, const char* serial0, const char* serial1, qcc::GUID128& guildID)
    {
        ECCPublicKey targetPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(targetBus, &targetPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipChainToTarget RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        ECCPublicKey secondPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(middleBus, &secondPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipChainToTarget RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        CredentialAccessor mca(middleBus);
        qcc::GUID128 middleGUID;
        status = mca.GetGuid(middleGUID);
        CredentialAccessor tca(targetBus);
        qcc::GUID128 targetGUID;
        status = tca.GetGuid(targetGUID);
        qcc::String middleCN((const char*) middleGUID.GetBytes(), middleGUID.SIZE);
        qcc::String targetCN((const char*) targetGUID.GetBytes(), targetGUID.SIZE);
        status = PermissionMgmtTestHelper::InstallMembershipChain(topBus, middleBus, serial0, serial1, targetBus.GetUniqueName().c_str(), middleCN, &secondPubKey, targetCN, &targetPubKey, guildID);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipChainToTarget failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install Membership to a consumer
     */
    void InstallOthersMembershipToConsumer()
    {
        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(adminBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallOthersMembershipToConsumer RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN((const char*) consumerGUID.GetBytes(), consumerGUID.SIZE);
        status = PermissionMgmtTestHelper::InstallMembership(membershipSerial1, adminBus, serviceBus.GetUniqueName(), adminBus, subjectCN, &claimedPubKey, membershipGUID1);

        EXPECT_EQ(ER_OK, status) << "  InstallOthersMembershipToConsumer InstallMembership failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install Membership to the admin
     */
    void InstallMembershipToAdmin(const GUID128& membershipGUID)
    {
        ECCPublicKey claimedPubKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(adminBus, &claimedPubKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToAdmin RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN((const char*) consumerGUID.GetBytes(), consumerGUID.SIZE);
        status = PermissionMgmtTestHelper::InstallMembership(membershipSerial1, adminProxyBus, adminBus.GetUniqueName(), adminBus, subjectCN, &claimedPubKey, membershipGUID);

        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToAdmin cert1 failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Install Membership to the admin
     */
    void InstallAdminMembershipToBus(BusAttachment& issuerBus, BusAttachment& targetBus, const char* adminGroupSerial, const GUID128& adminGroupGUID)
    {
        ECCPublicKey targetPublicKey;
        status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(targetBus, &targetPublicKey);
        EXPECT_EQ(ER_OK, status) << "  InstallMembershipToAdmin RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
        qcc::String subjectCN((const char*) consumerGUID.GetBytes(), consumerGUID.SIZE);
        status = PermissionMgmtTestHelper::InstallMembership(adminGroupSerial, issuerBus, targetBus.GetUniqueName(), targetBus, subjectCN, &targetPublicKey, adminGroupGUID);

        EXPECT_EQ(ER_OK, status) << "  InstallAdminMembershipToBus cert1 failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  App can call On
     */
    void AppCanCallOn(BusAttachment& bus, BusAttachment& targetBus)
    {
        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(bus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AppCanCallOn ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
        //bus.LeaveSession(clientProxyObject.GetSessionId());
    }

    /**
     *  App can't call On
     */
    void AppCannotCallOn(BusAttachment& bus, BusAttachment& targetBus)
    {
        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallOn ExcerciseOn did not fail.  Actual Status: " << QCC_StatusText(status);
        //bus.LeaveSession(clientProxyObject.GetSessionId());
    }

    /**
     *  any user can call TV On but not Off
     */
    void AnyUserCanCallOnAndNotOff(BusAttachment& bus)
    {

        ProxyBusObject clientProxyObject(bus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(bus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AnyUserCanCallOnAndNotOff ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseOff(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AnyUserCanCallOnAndNotOff ExcersizeOff did not fail.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  consumer can call TV On and Off
     */
    void ConsumerCanCallOnAndOff()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanCallOnAndOff ExcerciseOn failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseOff(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanCallOnAndOff ExcersizeOff failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  app can't call TV On
     */
    void AppCannotCallTVOn(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOn(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallTVOn ExcerciseOn should have failed.  Actual Status: " << QCC_StatusText(status);
    }

    void AppCannotCallTVDown(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVDown(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallTVDown ExcerciseTVDown should have failed.  Actual Status: " << QCC_StatusText(status);
    }

    void AppCanCallTVUp(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVUp(bus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AppCanCallTVUp ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  app can't call TV Off
     */
    void AppCannotCallTVOff(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOff(bus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  AppCannotCallTVOff ExcerciseOff should have failed.  Actual Status: " << QCC_StatusText(status);
    }

    void AppCanSetTVVolume(BusAttachment& bus, BusAttachment& targetBus, uint32_t tvVolume)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::SetTVVolume(bus, clientProxyObject, tvVolume)) << "  AppCanSetTVVolume SetTVVolume failed.";
        uint32_t newTvVolume = 0;
        EXPECT_EQ(ER_OK, PermissionMgmtTestHelper::GetTVVolume(bus, clientProxyObject, newTvVolume)) << "  AppCanSetTVVolume failed.";
        EXPECT_EQ(newTvVolume, tvVolume) << "  AppCanSetTVVolume GetTVVolume got wrong TV volume.";
    }

    /**
     *  Consumer can't call TV On
     */
    void ConsumerCannotCallTVOn()
    {
        AppCannotCallTVOn(consumerBus, serviceBus);
    }

    /**
     *  Consumer can't call TV InputSource
     */
    void ConsumerCannotCallTVInputSource()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVInputSource(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  ConsumerCannotCallTVInputSource ExcerciseTVInputSource should have failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  App gets the Security interfaces' version number
     */
    void AppGetVersionNumber(BusAttachment& bus, BusAttachment& targetBus)
    {
        uint16_t versionNum = 0;
        SecurityApplicationProxy saProxy(bus, targetBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, saProxy.GetSecurityApplicationVersion(versionNum)) << "AppGetVersionNumber GetSecurityApplicationVersion failed.";
        EXPECT_EQ(1, versionNum) << "AppGetVersionNumber received unexpected version number.";
        EXPECT_EQ(ER_OK, saProxy.GetClaimableApplicationVersion(versionNum)) << "AppGetVersionNumber GetClaimableApplicationVersion failed.";
        EXPECT_EQ(1, versionNum) << "AppGetVersionNumber received unexpected version number.";
        EXPECT_EQ(ER_OK, saProxy.GetManagedApplicationVersion(versionNum)) << "AppGetVersionNumber GetClaimableApplicationVersion failed.";
        EXPECT_EQ(1, versionNum) << "AppGetVersionNumber received unexpected version number.";
    }

    /**
     *  App can call TV Off
     */
    void AppCanCallTVOff(BusAttachment& bus, BusAttachment& targetBus)
    {

        ProxyBusObject clientProxyObject(bus, targetBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseOff(bus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AppCanCallTVOff ExcerciseOff failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Consumer can call TV Off
     */
    void ConsumerCanCallTVOff()
    {
        AppCanCallTVOff(consumerBus, serviceBus);
    }

    /**
     *  Guild member can turn up/down but can't specify a channel
     */
    void ConsumerCanTVUpAndDownAndNotChannel()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVUp(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseTVDown(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel ExcerciseTVDown failed.  Actual Status: " << QCC_StatusText(status);
        status = PermissionMgmtTestHelper::ExcerciseTVChannel(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel ExcerciseTVChannel did not fail.  Actual Status: " << QCC_StatusText(status);

        uint32_t tvVolume = 35;
        status = PermissionMgmtTestHelper::SetTVVolume(consumerBus, clientProxyObject, tvVolume);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel SetTVVolume failed.  Actual Status: " << QCC_StatusText(status);
        uint32_t newTvVolume = 0;
        status = PermissionMgmtTestHelper::GetTVVolume(consumerBus, clientProxyObject, newTvVolume);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanTVUpAndDownAndNotChannel GetTVVolume failed.  Actual Status: " << QCC_StatusText(status);
        EXPECT_EQ(newTvVolume, tvVolume) << "  ConsumerCanTVUpAndDownAndNotChannel GetTVVolume got wrong TV volume.";
    }

    /**
     *  consumer cannot turn TV up
     */
    void ConsumerCannotTurnTVUp()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::ExcerciseTVUp(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  ConsumerCannotTurnTVUp ExcerciseTVUp failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  consumer cannot get the TV caption
     */
    void ConsumerCannotGetTVCaption()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::GetTVCaption(consumerBus, clientProxyObject);
        EXPECT_NE(ER_OK, status) << "  ConsumerCannotGetTVCaption GetTVCaption did not fail.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  consumer can get the TV caption
     */
    void ConsumerCanGetTVCaption()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        QStatus status = PermissionMgmtTestHelper::GetTVCaption(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCannotGetTVCaption GetTVCaption failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  Admin can change channel
     */
    void AdminCanChangeChannlel()
    {

        ProxyBusObject clientProxyObject(adminBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        status = PermissionMgmtTestHelper::ExcerciseTVChannel(adminBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  AdminCanChangeChannlel failed.  Actual Status: " << QCC_StatusText(status);
    }

    /**
     *  consumer can change channel
     */
    void ConsumerCanChangeChannlel()
    {

        ProxyBusObject clientProxyObject(consumerBus, serviceBus.GetUniqueName().c_str(), GetPath(), 0, false);
        status = PermissionMgmtTestHelper::ExcerciseTVChannel(consumerBus, clientProxyObject);
        EXPECT_EQ(ER_OK, status) << "  ConsumerCanChangeChannlel failed.  Actual Status: " << QCC_StatusText(status);
    }

    /*
     *  Set the manifest for the service provider
     */
    void SetPermissionManifestOnServiceProvider()
    {

        PermissionPolicy::Rule* rules = NULL;
        size_t count = 0;
        QStatus status = GenerateManifestTemplate(&rules, &count);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest GenerateManifest failed.  Actual Status: " << QCC_StatusText(status);
        PermissionConfigurator& pc = serviceBus.GetPermissionConfigurator();
        status = pc.SetPermissionManifest(rules, count);
        EXPECT_EQ(ER_OK, status) << "  SetPermissionManifest SetPermissionManifest failed.  Actual Status: " << QCC_StatusText(status);

        PermissionMgmtProxy pmProxy(adminBus, serviceBus.GetUniqueName().c_str());
        PermissionPolicy::Rule* retrievedRules = NULL;
        size_t retrievedCount = 0;
        EXPECT_EQ(ER_OK, pmProxy.GetManifest(&retrievedRules, &retrievedCount)) << "SetPermissionManifest GetManifest failed.";
        EXPECT_EQ(count, retrievedCount) << "  SetPermissionManifest GetManifest failed to retrieve the same count.";
        delete [] rules;
        delete [] retrievedRules;
    }

    /*
     *  Remove Policy from service provider
     */
    void RemovePolicyFromServiceProvider()
    {
        PermissionMgmtProxy pmProxy(adminBus, serviceBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, pmProxy.RemovePolicy()) << "RemovePolicy failed.";
        /* get policy again.  Expect it to fail */
        PermissionPolicy retPolicy;
        /* sleep a second to see whether the ApplicationState signal is received */
        EXPECT_NE(ER_OK, pmProxy.GetPolicy(&retPolicy)) << "GetPolicy did not fail.";
        for (int cnt = 0; cnt < 100; cnt++) {
            if (GetApplicationStateSignalReceived()) {
                break;
            }
            qcc::Sleep(10);
        }
        EXPECT_TRUE(GetApplicationStateSignalReceived()) << " Fail to receive expected ApplicationState signal.";
    }

    /*
     * Remove Membership from consumer.
     */
    void RemoveMembershipFromConsumer()
    {
        PermissionMgmtProxy pmProxy(adminBus, consumerBus.GetUniqueName().c_str());
        qcc::GUID128 issuerGUID;
        PermissionMgmtTestHelper::GetGUID(adminBus, issuerGUID);
        KeyInfoNISTP256 keyInfo;
        adminBus.GetPermissionConfigurator().GetSigningPublicKey(keyInfo);
        String aki;
        CertificateX509::GenerateAuthorityKeyId(keyInfo.GetPublicKey(), aki);

        EXPECT_EQ(ER_OK, pmProxy.RemoveMembership(membershipSerial1, aki)) << "RemoveMembershipFromConsumer failed.";
        /* removing it again */
        EXPECT_NE(ER_OK, pmProxy.RemoveMembership(membershipSerial1, aki)) << "RemoveMembershipFromConsumer succeeded.  Expect it to fail.";

    }

    /**
     * Test PermissionMgmt Reset method on service.  The consumer should not be
     * able to reset the service since the consumer is not an admin.
     */
    void FailResetServiceByConsumer()
    {
        PermissionMgmtProxy pmProxy(consumerBus, serviceBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_PERMISSION_DENIED, pmProxy.Reset()) << "  Reset is not supposed to succeed.";

    }

    /*
     * Test PermissionMgmt Reset method on service by the admin.  The admin should be
     * able to reset the service.
     */
    void SuccessfulResetServiceByAdmin()
    {
        PermissionMgmtProxy pmProxy(adminBus, serviceBus.GetUniqueName().c_str());
        EXPECT_EQ(ER_OK, pmProxy.Reset()) << "  Reset failed.";

        /* retrieve the current identity cert */
        IdentityCertificate* certs = NULL;
        size_t count = 0;
        EXPECT_NE(ER_OK, pmProxy.GetIdentity(&certs, &count)) << "GetIdentity is not supposed to succeed since it was removed by Reset.";
    }

    /*
     * retrieve the peer public key.
     */
    void RetrieveServicePublicKey()
    {
        PermissionConfigurator& pc = consumerBus.GetPermissionConfigurator();
        GUID128 serviceGUID(0);
        String peerName = serviceBus.GetUniqueName();
        status = PermissionMgmtTestHelper::GetPeerGUID(consumerBus, peerName, serviceGUID);
        EXPECT_EQ(ER_OK, status) << "  ca.GetPeerGuid failed.  Actual Status: " << QCC_StatusText(status);
        ECCPublicKey publicKey;
        status = pc.GetConnectedPeerPublicKey(serviceGUID, &publicKey);
        EXPECT_EQ(ER_OK, status) << "  GetConnectedPeerPublicKey failed.  Actual Status: " << QCC_StatusText(status);
    }

    /*
     * retrieve the peer public key.
     */
    void ClearPeerKeys(BusAttachment& bus, BusAttachment& peerBus)
    {
        String peerName = peerBus.GetUniqueName();
        GUID128 peerGUID(0);
        QStatus status = PermissionMgmtTestHelper::GetPeerGUID(bus, peerName, peerGUID);
        EXPECT_EQ(ER_OK, status) << "  PermissionMgmtTestHelper::GetPeerGuid failed.  Actual Status: " << QCC_StatusText(status);
        status = bus.ClearKeys(peerGUID.ToString());
        EXPECT_EQ(ER_OK, status) << "  BusAttachment::ClearKeys failed.  Actual Status: " << QCC_StatusText(status);
    }
};

class PathBasePermissionMgmtUseCaseTest : public PermissionMgmtUseCaseTest {
  public:
    PathBasePermissionMgmtUseCaseTest() : PermissionMgmtUseCaseTest("/control/guide")
    {
    }
};

/*
 *  Test all the possible calls provided PermissionMgmt interface
 */
TEST_F(PermissionMgmtUseCaseTest, TestAllCalls)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    ReplaceIdentityCert(adminBus, serviceBus);
    InstallMembershipToServiceProvider();

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    SetChannelChangedSignalReceived(false);
    ConsumerCanTVUpAndDownAndNotChannel();
    ConsumerCanGetTVCaption();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetChannelChangedSignalReceived()) << " Fail to receive expected ChannelChanged signal.";

    SetPermissionManifestOnServiceProvider();

    RetrieveServicePublicKey();
    RemoveMembershipFromServiceProvider();
    RemoveMembershipFromConsumer();
    FailResetServiceByConsumer();
    SuccessfulResetServiceByAdmin();
    AppGetVersionNumber(consumerBus, serviceBus);
}

/*
 *  Case: claiming, install policy, install membership, and access
 */
TEST_F(PermissionMgmtUseCaseTest, ClaimPolicyMembershipAccess)
{
    Claims(true);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();
    SetPermissionManifestOnServiceProvider();

}

/*
 *  Case: outbound message allowed by guild based acls and peer's membership
 */
TEST_F(PathBasePermissionMgmtUseCaseTest, OutboundAllowedByMembership)
{
    Claims(true);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    InstallMembershipToServiceProvider("1234", membershipGUID1);

    policy = GenerateGuildSpecificAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority, membershipGUID1, consumerBus);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();
}

/*
 *  Case: outbound message not allowed by guild based acls since
 *       peer does not have given guild membership.
 */
TEST_F(PermissionMgmtUseCaseTest, OutboundNotAllowedByMissingPeerMembership)
{
    Claims(true);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateGuildSpecificAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority, membershipGUID1, consumerBus);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();

}

/*
 *  Service Provider has no policy: claiming, access
 */
TEST_F(PermissionMgmtUseCaseTest, ClaimNoPolicyAccess)
{
    Claims(true);
    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotCallTVOn();
}

TEST_F(PermissionMgmtUseCaseTest, AccessByPublicKey)
{
    Claims(true);
    /* generate a policy */
    ECCPublicKey consumerPublicKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &consumerPublicKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicyPeerPublicKey(adminAdminGroupGUID, adminAdminGroupAuthority, consumerPublicKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanTVUpAndDownAndNotChannel();
}

TEST_F(PermissionMgmtUseCaseTest, AccessDeniedForPeerPublicKey)
{
    Claims(true);

    /* generate a policy */
    ECCPublicKey consumerPublicKey;
    status = PermissionMgmtTestHelper::RetrieveDSAPublicKeyFromKeyStore(consumerBus, &consumerPublicKey);
    EXPECT_EQ(ER_OK, status) << "  RetrieveDSAPublicKeyFromKeyStore failed.  Actual Status: " << QCC_StatusText(status);
    PermissionPolicy* policy = GeneratePolicyDenyPeerPublicKey(adminAdminGroupGUID, adminAdminGroupAuthority, consumerPublicKey);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AppCanCallTVUp(consumerBus, serviceBus);
    AppCannotCallTVDown(consumerBus, serviceBus);
}

TEST_F(PermissionMgmtUseCaseTest, AdminHasFullAccess)
{
    Claims(true);

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(adminBus, false);

    AdminCanChangeChannlel();
}

/*
 *  Case: unclaimed app does not have restriction
 */
TEST_F(PermissionMgmtUseCaseTest, UnclaimedProviderAllowsEverything)
{
    EnableSecurity("ALLJOYN_ECDHE_PSK");

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanChangeChannlel();
}

TEST_F(PermissionMgmtUseCaseTest, InstallIdentityCertWithDifferentPubKey)
{
    Claims(false);
    ReplaceIdentityCertWithBadPublicKey(adminBus, serviceBus);
}

TEST_F(PermissionMgmtUseCaseTest, InstallIdentityCertWithExpiredCert)
{
    Claims(false);
    ReplaceIdentityCertWithExpiredCert(adminBus, consumerBus);
}

/*
 *  Case: claiming, install policy, install wrong membership, and fail access
 */
TEST_F(PermissionMgmtUseCaseTest, SendingOthersMembershipCert)
{
    Claims(true);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    InstallOthersMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCannotTurnTVUp();
}

TEST_F(PermissionMgmtUseCaseTest, AccessNotAuthorizedBecauseOfWrongActionMask)
{
    Claims(true);
    /* generate a limited policy */
    PermissionPolicy* policy = GenerateSmallAnyUserPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
}

TEST_F(PermissionMgmtUseCaseTest, AccessNotAuthorizedBecauseOfDeniedOnPrefix)
{
    Claims(true);
    /* generate a limited policy */
    PermissionPolicy* policy = GenerateAnyUserDeniedPrefixPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
}

TEST_F(PathBasePermissionMgmtUseCaseTest, ProviderHasNoMatchingGuildForConsumer)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    InstallMembershipToServiceProvider();

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer(membershipSerial4, membershipGUID4, adminBus);
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();
}

TEST_F(PermissionMgmtUseCaseTest, ProviderHasMoreMembershipCertsThanConsumer)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    InstallMembershipToServiceProvider();

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCanTVUpAndDownAndNotChannel();

}

TEST_F(PermissionMgmtUseCaseTest, ConsumerHasMoreMembershipCertsThanService)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    InstallMembershipToServiceProvider();

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    InstallMembershipToConsumer(membershipSerial2, membershipGUID2, adminBus);
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanCallTVOff();
}

TEST_F(PermissionMgmtUseCaseTest, ConsumerHasGoodMembershipCertChain)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    InstallMembershipToServiceProvider();

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipChainToTarget(adminBus, adminBus, consumerBus, membershipSerial0, membershipSerial1, membershipGUID1);

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanTVUpAndDownAndNotChannel();
}

TEST_F(PermissionMgmtUseCaseTest, ConsumerHasMoreRestrictiveManifest)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    InstallMembershipToServiceProvider();

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipChainToTarget(adminBus, adminBus, consumerBus, membershipSerial0, membershipSerial1, membershipGUID1);

    PermissionPolicy::Rule* manifest;
    size_t manifestSize;
    GenerateManifestNoInputSource(&manifest, &manifestSize);
    ReplaceIdentityCert(adminBus, consumerBus, manifest, manifestSize, false);
    delete [] manifest;

    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanTVUpAndDownAndNotChannel();
    ConsumerCannotCallTVInputSource();
}

TEST_F(PathBasePermissionMgmtUseCaseTest, ConsumerHasLessAccessInManifestUsingDenied)
{
    Claims(false);
    /* generate a policy */
    PermissionPolicy* policy = GeneratePolicy(adminAdminGroupGUID, adminAdminGroupAuthority, consumerBus);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;
    InstallMembershipToServiceProvider();

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();

    PermissionPolicy::Rule* manifest;
    size_t manifestSize;
    GenerateManifestDenied(true, true, &manifest, &manifestSize);
    ReplaceIdentityCert(adminBus, consumerBus, manifest, manifestSize, false);
    delete [] manifest;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    AnyUserCanCallOnAndNotOff(consumerBus);
    ConsumerCannotTurnTVUp();
    ConsumerCannotGetTVCaption();
}

TEST_F(PermissionMgmtUseCaseTest, AllowEverything)
{
    Claims(true);
    /* generate a limited policy */
    PermissionPolicy* policy = GenerateWildCardPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;

    InstallMembershipToConsumer();
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    ConsumerCanCallOnAndOff();
}

TEST_F(PermissionMgmtUseCaseTest, SignalAllowedFromAnyUser)
{
    Claims(false);
    /* generate a policy to permit sending a signal */
    PermissionPolicy* policy = GenerateFullAccessAnyUserPolicy(adminAdminGroupGUID, adminAdminGroupAuthority, true);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    SetChannelChangedSignalReceived(false);
    ConsumerCanChangeChannlel();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_TRUE(GetChannelChangedSignalReceived()) << " Fail to receive expected ChannelChanged signal.";
}

TEST_F(PermissionMgmtUseCaseTest, SignalNotAllowedToEmit)
{
    Claims(false);
    /* generate a policy not permit sending a signal */
    PermissionPolicy* policy = GenerateFullAccessAnyUserPolicy(adminAdminGroupGUID, adminAdminGroupAuthority, false);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    InstallPolicyToConsumer(*policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    SetChannelChangedSignalReceived(false);
    ConsumerCanChangeChannlel();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_FALSE(GetChannelChangedSignalReceived()) << " Unexpect to receive ChannelChanged signal.";
}

TEST_F(PermissionMgmtUseCaseTest, SignalNotAllowedToReceive)
{
    Claims(false);
    /* generate a policy to permit sending a signal */
    PermissionPolicy* policy = GenerateFullAccessAnyUserPolicy(adminAdminGroupGUID, adminAdminGroupAuthority, true);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    /* full access outgoing but do not accept incoming signal */
    policy = GenerateFullAccessOutgoingPolicy(adminAdminGroupGUID, adminAdminGroupAuthority, false);
    InstallPolicyToConsumer(*policy);
    delete policy;
    /* setup the application interfaces for access tests */
    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(consumerBus, false);

    SetChannelChangedSignalReceived(false);
    ConsumerCanChangeChannlel();
    /* sleep a second to see whether the ChannelChanged signal is received */
    for (int cnt = 0; cnt < 100; cnt++) {
        if (GetChannelChangedSignalReceived()) {
            break;
        }
        qcc::Sleep(10);
    }
    EXPECT_FALSE(GetChannelChangedSignalReceived()) << " Unexpect to receive ChannelChanged signal.";
}

TEST_F(PermissionMgmtUseCaseTest, AccessGrantedForPeerFromSpecificCA)
{
    Claims(true);  /* also claim the remote control */

    /* Setup so remote control is not trusted by service provider */
    PermissionPolicy* policy = GenerateSmallAnyUserPolicy(adminAdminGroupGUID, adminAdminGroupAuthority);
    ASSERT_TRUE(policy) << "GeneratePolicy failed.";
    InstallPolicyToService(*policy);
    delete policy;

    policy = GenerateFullAccessOutgoingPolicy(consumerAdminGroupGUID, consumerAdminGroupAuthority);
    InstallPolicyToClientBus(consumerBus, remoteControlBus, *policy);
    delete policy;

    CreateAppInterfaces(serviceBus, true);
    CreateAppInterfaces(remoteControlBus, false);

    /* access expected to fail because of lack of trust */
    AppCannotCallOn(remoteControlBus, serviceBus);

    /* Set up the service provider to trust the remote control via the peer
     * type FROM_CERTIFICATE_AUTHORITY.
     */
    ClearPeerKeys(remoteControlBus, serviceBus);
    ClearPeerKeys(serviceBus, remoteControlBus);
    EnableSecurity("ALLJOYN_ECDHE_ECDSA");
    KeyInfoNISTP256 consumerCA;
    consumerBus.GetPermissionConfigurator().GetSigningPublicKey(consumerCA);
    policy = GeneratePolicyForSpecificCA(adminAdminGroupGUID, adminAdminGroupAuthority, consumerCA);
    ASSERT_TRUE(policy) << "GeneratePolicyForSpecificCA failed.";
    InstallPolicyToService(*policy);
    delete policy;
    InstallMembershipToServiceProvider();

    KeyInfoNISTP256 adminCA;
    adminBus.GetPermissionConfigurator().GetSigningPublicKey(adminCA);
    policy = GenerateFullAccessOutgoingPolicyWithGuestServices(consumerAdminGroupGUID, consumerAdminGroupAuthority, false, adminCA);
    InstallPolicyToClientBus(consumerBus, remoteControlBus, *policy);
    delete policy;

    /* remote control can access the service provider */
    AnyUserCanCallOnAndNotOff(remoteControlBus);
    /* remote control can access specific right for the given CA */
    AppCanSetTVVolume(remoteControlBus, serviceBus, 21);
}

