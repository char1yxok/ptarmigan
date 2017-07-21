/*
 *  Copyright (C) 2017, Nayuta, Inc. All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */
/** @file   ln.h
 *  @brief  Lightning
 *  @author ueno@nayuta.co
 *  @todo
 *      - 外部公開するAPIや構造体を整理する
 */
#ifndef LN_H__
#define LN_H__

#include <stdint.h>
#include <stdbool.h>

#include "ucoin.h"


#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**************************************************************************
 * macros
 **************************************************************************/

#define LN_SZ_CHANNEL_ID                (32)        ///< サイズ:channel_id
#define LN_SZ_SHORT_CHANNEL_ID          (8)         ///< サイズ:short_channel_id
#define LN_SZ_SIGNATURE                 (64)        ///< サイズ:署名
#define LN_SZ_HASH                      (32)        ///< サイズ:xxx-hash
#define LN_SZ_PREIMAGE                  (32)        ///< サイズ:preimage
#define LN_SZ_ONION_ROUTE               (1366)      ///< サイズ:onion-routing-packet
#define LN_SZ_ALIAS                     (32)        ///< サイズ:alias長
#define LN_SZ_NOISE_HEADER              (sizeof(uint16_t) + 16)     ///< サイズ:noiseパケットヘッダ

#define LN_FUNDIDX_MAX                  (5)         ///< 管理用
#define LN_SCRIPTIDX_MAX                (3)         ///< 管理用
#define LN_HTLC_MAX                     (6)         ///< 自分のHTLC数   TODO:暫定
                                                    //      max_accepted_htlcsとして使用する
                                                    //      相手の分も同じ分しか用意していない
                                                    //      相手からの要求はmax_accepted_htlcsまでしか受け入れないので、
                                                    //      こちらから要求しなければ済む話である。
#define LN_NODE_MAX                     (5)         ///< 保持するノード情報数   TODO:暫定
#define LN_CHANNEL_MAX                  (10)        ///< 保持するチャネル情報数 TODO:暫定
#define LN_HOP_MAX                      (20)        ///< onion hop数

// ln_update_add_htlc_t.flag用
#define LN_HTLC_FLAG_IS_RECV(f)         ((f) & LN_HTLC_FLAG_RECV)   ///< true:Received HTLC / false:Offered HTLC
#define LN_HTLC_FLAG_SEND               (0x00)                      ///< Offered HTLC(add_htlcを送信した)
#define LN_HTLC_FLAG_RECV               (0x01)                      ///< Received HTLC(add_htlcを受信した)


/**************************************************************************
 * macro functions
 **************************************************************************/

/** @def    LN_SEQUENCE(obs)
 *  @brief  obscured commitment numberから<sequence>算出
 */
#define LN_SEQUENCE(obs)        ((uint32_t)(0x80000000 | (((obs) >> 24) & 0xffffff))) //[0x80][上位3byte]


/** @def    LN_LOCKTIME(obs)
 *  @brief  obscured commitment numberから<locktime>算出
 */
#define LN_LOCKTIME(obs)        ((uint32_t)(0x20000000 | ((obs) & 0xffffff)))         //[0x20][下位3byte]


/** @def    LN_SATOSHI2MSAT(obs)
 *  @brief  satoshiをmsat(milli-satoshi)変換
 */
#define LN_SATOSHI2MSAT(sat)    ((uint64_t)(sat) * (uint64_t)1000)


/** @def    LN_MSAT2SATOSHI(obs)
 *  @brief  msat(milli-satoshi)をsatoshi変換
 *  @note
 *      - LNプロトコル仕様により、32bitまでの箇所があるので注意すること
 */
#define LN_MSAT2SATOSHI(msat)   ((msat) / 1000)


/********************************************************************
 * typedefs
 ********************************************************************/

//forward definition
struct ln_self_t;
typedef struct ln_self_t ln_self_t;


/** @enum   ln_cb_t
 *  @brief  コールバック理由
 */
typedef enum {
    LN_CB_ERROR,                ///< エラー通知
    LN_CB_INIT_RECV,            ///< init受信通知
    LN_CB_FINDINGWIF_REQ,       ///< funding鍵設定要求
    LN_CB_FUNDINGTX_WAIT,       ///< funding_tx安定待ち要求
    LN_CB_ESTABLISHED,          ///< Establish完了通知
    LN_CB_NODE_ANNO_RECV,       ///< node_announcement受信通知
    LN_CB_ANNO_SIGNS_RECV,      ///< announcement_signatures受信通知
    LN_CB_ADD_HTLC_RECV_PREV,   ///< update_add_htlc処理前通知
    LN_CB_ADD_HTLC_RECV,        ///< update_add_htlc受信通知
    LN_CB_FULFILL_HTLC_RECV,    ///< update_fulfill_htlc受信通知
    LN_CB_HTLC_CHANGED,         ///< HTLC変化通知
    LN_CB_CLOSED,               ///< closing_signed受信通知
    LN_CB_SEND_REQ,             ///< peerへの送信要求
    LN_CB_COMMIT_SIG_RECV,      ///< commitment_signed受信通知
    LN_CB_MAX,
} ln_cb_t;


/** @typedef    ln_callback_t
 *  @brief      通知コールバック関数
 */
typedef void (*ln_callback_t)(ln_self_t*, ln_cb_t, void *);


/**************************************************************************
 * typedefs : HTLC
 **************************************************************************/

/** @enum   ln_htlctype_t
 *  @brief  HTLC種別
 */
typedef enum {
    LN_HTLCTYPE_NONE,                               ///< 未設定
    LN_HTLCTYPE_OFFERED,                            ///< Offered HTLC
    LN_HTLCTYPE_RECEIVED,                           ///< Received HTLC
} ln_htlctype_t;


/** @struct ln_feeinfo_t
 *  @brief  FEE情報
 */
typedef struct {
    uint32_t        feerate_per_kw;                 ///< [IN]1000byte辺りのsatoshi
    uint64_t        dust_limit_satoshi;             ///< [IN]dust_limit_satoshi

    uint64_t        htlc_success;                   ///< [CALC]HTLC success Transaction FEE
    uint64_t        htlc_timeout;                   ///< [CALC]HTLC timeout Transaction FEE
    uint64_t        commit;                         ///< [CALC]Commitment Transaction FEE
} ln_feeinfo_t;


/** @struct ln_htlcinfo_t
 *  @brief  HTLC情報
 */
typedef struct {
    ln_htlctype_t           type;                   ///< HTLC種別
    uint32_t                expiry;                 ///< Expiry
    uint64_t                amount_msat;            ///< amount_msat
    const uint8_t           *preimage;              ///< preimage(Offeredか、相手から取得できた場合)
    const uint8_t           *preimage_hash;         ///< preimageをHASH160したデータ
    ucoin_buf_t             script;                 ///< スクリプト
} ln_htlcinfo_t;


/** @struct ln_tx_cmt_t
 *  @brief  Commitment Transaction生成用情報
 */
typedef struct {
    struct {
        const uint8_t       *txid;              ///< funding txid
        uint32_t            txid_index;         ///< funding txid index
        uint64_t            satoshi;            ///< funding satoshi
        const ucoin_buf_t   *p_script;          ///< funding script
        ucoin_util_keys_t   *p_keys;            ///< funding local keys(remoteは署名をもらう)
    } fund;

    struct {
        uint64_t            satoshi;            ///< local satoshi
        const ucoin_buf_t   *p_script;          ///< to-local script
    } local;
    struct {
        uint64_t            satoshi;            ///< remote satoshi
        const uint8_t       *pubkey;            ///< remote pubkey(to-remote用)
    } remote;

    uint64_t                obscured;           ///< Obscured Commitment Number(ln_calc_obscured_txnum())
    ln_feeinfo_t            *p_feeinfo;         ///< FEE情報
    ln_htlcinfo_t           **pp_htlcinfo;      ///< HTLC情報ポインタ配列(htlcinfo_num個分)
    uint8_t                 htlcinfo_num;       ///< HTLC数
} ln_tx_cmt_t;


/** @struct ln_derkey_storage
 *  @brief  per-commitment secret storage
 *      https://github.com/nayuta-ueno/lightning-rfc/blob/master/03-transactions.md#efficient-per-commitment-secret-storage
 */
typedef struct {
    struct {
        uint8_t     secret[UCOIN_SZ_PRIVKEY];   ///< secret
        uint64_t    index;                      ///< index
    } storage[49];
} ln_derkey_storage;


/**************************************************************************
 * typedefs : Establish channel
 **************************************************************************/

/// @addtogroup channel_establish
/// @{

/** @struct ln_open_channel_t
 *  @brief  [Establish]open_channel
 */
typedef struct {
    uint64_t    funding_sat;                        ///< 8 : funding-satoshis
    uint64_t    push_msat;                          ///< 8 : push-msat
    uint64_t    dust_limit_sat;                     ///< 8 : dust-limit-satoshis
    uint64_t    max_htlc_value_in_flight_msat;      ///< 8 : max-htlc-value-in-flight-msat
    uint64_t    channel_reserve_sat;                ///< 8 : channel-reserve-satoshis
    uint64_t    htlc_minimum_msat;                  ///< 8 : htlc-minimum-msat
    uint32_t    feerate_per_kw;                     ///< 4 : feerate-per-kw
    uint16_t    to_self_delay;                      ///< 2 : to-self-delay
    uint16_t    max_accepted_htlcs;                 ///< 2 : max-accepted-htlcs

    uint8_t     *p_temp_channel_id;                 ///< 32: temporary-channel-id
    uint8_t     *p_pubkeys[LN_FUNDIDX_MAX];         ///< 33: [0]funding-pubkey
                                                    ///< 33: [1]revocation-basepoint
                                                    ///< 33: [2]payment-basepoint
                                                    ///< 33: [3]delayed-payment-basepoint
                                                    ///< 33: [4]first-per-commitment-point
} ln_open_channel_t;


/** @struct ln_accept_channel_t
 *  @brief  [Establish]accept_channel
 */
typedef struct {
    uint64_t    dust_limit_sat;                     ///< 8 : dust-limit-satoshis
    uint64_t    max_htlc_value_in_flight_msat;      ///< 8 : max-htlc-value-in-flight-msat
    uint64_t    channel_reserve_sat;                ///< 8 : channel-reserve-satoshis
    uint64_t    htlc_minimum_msat;                  ///< 8 : htlc-minimum-msat
    uint32_t    min_depth;                          ///< 4 : minimum-depth(acceptのみ)
    uint16_t    to_self_delay;                      ///< 2 : to-self-delay
    uint16_t    max_accepted_htlcs;                 ///< 2 : max-accepted-htlcs

    uint8_t     *p_temp_channel_id;                 ///< 32: temporary-channel-id
    uint8_t     *p_pubkeys[LN_FUNDIDX_MAX];         ///< 33: [0]funding-pubkey
                                                    ///< 33: [1]revocation-basepoint
                                                    ///< 33: [2]payment-basepoint
                                                    ///< 33: [3]delayed-payment-basepoint
                                                    ///< 33: [4]first-per-commitment-point
} ln_accept_channel_t;


/** @struct ln_funding_created_t
 *  @brief  [Establish]funding_created
 */
typedef struct {
    uint16_t    funding_output_idx;                 ///< 2:  funding-output-index

    uint8_t     *p_temp_channel_id;                 ///< 32: temporary-channel-id
    uint8_t     *p_funding_txid;                    ///< 32: funding-txid
    uint8_t     *p_signature;                       ///< 64: signature
} ln_funding_created_t;


/** @struct ln_funding_signed_t
 *  @brief  [Establish]funding_signed
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint8_t     *p_signature;                       ///< 64: signature
} ln_funding_signed_t;


/** @struct ln_funding_locked_t
 *  @brief  [Establish]funding_locked
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint8_t     *p_per_commitpt;                    ///< 33: next-per-commitment-point
} ln_funding_locked_t;


/** @struct ln_fundin_t
 *  @brief  open_channelでのfund_in情報
 *  @note
 *      - open_channelする方が #ln_establish_t .p_fundinに設定して使う
 */
typedef struct {
    const uint8_t               *p_txid;                        ///< 2-of-2へ入金するTXID
    int32_t                     index;                          ///< 未設定時(channelを開かれる方)は-1
    uint64_t                    amount;                         ///< 2-of-2へ入金するtxのvout amount
    const uint8_t               *p_change_pubkey;               ///< 2-of-2へ入金したお釣りの送金先アドレス
    const char                  *p_change_addr;                 ///< 2-of-2へ入金したお釣りの送金先アドレス
    const ucoin_util_keys_t     *p_keys;                        ///< 2-of-2へ入金するtxの鍵(署名用)
    bool                        b_native;                       ///< true:fundinがnative segwit output
} ln_fundin_t;


/** @struct ln_default_t
 *  @brief  Establish関連のデフォルト値
 *  @note
 *      - #ln_set_establish()で初期化する
 */
typedef struct {
    uint64_t    dust_limit_sat;                     ///< 8 : dust-limit-satoshis
    uint64_t    max_htlc_value_in_flight_msat;      ///< 8 : max-htlc-value-in-flight-msat
    uint64_t    channel_reserve_sat;                ///< 8 : channel-reserve-satoshis
    uint64_t    htlc_minimum_msat;                  ///< 8 : htlc-minimum-msat
    uint32_t    feerate_per_kw;                     ///< 4 : feerate-per-kw
    uint16_t    to_self_delay;                      ///< 2 : to-self-delay
    uint16_t    max_accepted_htlcs;                 ///< 2 : max-accepted-htlcs
    uint32_t    min_depth;                          ///< 4 : minimum-depth(acceptのみ)
} ln_default_t;


/** @struct ln_establish_t
 *  @brief  [Establish]ワーク領域
 */
typedef struct {
    ln_open_channel_t           cnl_open;                       ///< 送信 or 受信したopen_channel
    ln_accept_channel_t         cnl_accept;                     ///< 送信 or 受信したaccept_channel
    ln_funding_created_t        cnl_funding_created;            ///< 送信 or 受信したfunding_created
    ln_funding_signed_t         cnl_funding_signed;             ///< 送信 or 受信したfunding_signed

    const ln_fundin_t           *p_fundin;                      ///< 非NULL:open_channel側
    ln_default_t                defval;                         ///< デフォルト値
} ln_establish_t;

/// @}


/**************************************************************************
 * typedefs : Channel Close
 **************************************************************************/

/// @addtogroup channel_close
/// @{

/** @struct ln_shutdown_t
 *  @brief  [Close]shutdown
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    ucoin_buf_t *p_scriptpk;                        ///< len: scriptpubkey
} ln_shutdown_t;


/** @struct ln_closing_signed_t
 *  @brief  [Close]closing_signed
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint64_t    fee_sat;                            ///< 8:  fee-satoshis
    uint8_t     *p_signature;                       ///< 64: signature
} ln_closing_signed_t;

/// @}


/**************************************************************************
 * typedefs : Normal Operation
 **************************************************************************/

/// @addtogroup normal_operation
/// @{

/** @struct     ln_update_add_htlc_t
 *  @brief      update_add_htlc
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint64_t    id;                                 ///< 8:  id
    uint64_t    amount_msat;                        ///< 8:  amount-msat
    uint32_t    cltv_expiry;                        ///< 4:  cltv-expirty
    uint8_t     payment_sha256[LN_SZ_HASH];         ///< 32: payment-hash
    uint8_t     *p_onion_route;                     ///< 1366: onion-routing-packet
    //inner
    uint8_t     flag;                               ///< LN_HTLC_FLAG_xxx
    //fulfillで戻す
    uint8_t     signature[LN_SZ_SIGNATURE];         ///< HTLC署名
    uint64_t    prev_short_channel_id;              ///< 転送元short_channel_id
} ln_update_add_htlc_t;


/** @struct     ln_update_fulfill_htlc_t
 *  @brief      update_fulfill_htlc
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint64_t    id;                                 ///< 8:  id
    uint8_t     *p_payment_preimage;                ///< 32: payment-preimage
} ln_update_fulfill_htlc_t;


/** @struct     ln_update_fail_htlc_t
 *  @brief      update_fail_htlc
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint64_t    id;                                 ///< 8:  id
    ucoin_buf_t *p_reason;                          ///< len: reason
} ln_update_fail_htlc_t;


/** @struct     ln_commit_signed_t
 *  @brief      commitment_signed
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint8_t     *p_signature;                       ///< 64: signature
    uint16_t    num_htlcs;                          ///< 2:  num-htlcs
    uint8_t     *p_htlc_signature;                  ///< num-htlcs*64: htlc-signature
} ln_commit_signed_t;


/** @struct     ln_revoke_and_ack_t
 *  @brief      revoke_and_ack
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint8_t     *p_per_commit_secret;               ///< 32: 古いper-commiment-secret
    uint8_t     *p_per_commitpt;                    ///< 33: 新しいper-commtment-point
} ln_revoke_and_ack_t;


/** @struct     ln_update_fee_t
 *  @brief      update_fee
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint32_t    feerate_per_kw;                     ///< 4:  feerate-per-kw
} ln_update_fee_t;


/** @struct     ln_update_fail_malformed_htlc_t
 *  @brief      update_fail_malformed_htlc
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint64_t    id;                                 ///< 8:  id
    uint8_t     *p_sha256_onion;                    ///< 32: sha256-of-onion
    uint16_t    failure_code;                       ///< 2:  failure-code
} ln_update_fail_malformed_htlc_t;

/// @}


/**************************************************************************
 * typedefs : Setup/Control
 **************************************************************************/

/// @addtogroup setup_control
/// @{

/** @struct     ln_init_t
 *  @brief      init
 */
typedef struct {
    //uint8_t     globalfeatures;                     ///< 1: globalfeatures
    uint8_t     localfeatures;                      ///< 1: localfeatures
} ln_init_t;


/** @struct     ln_ping_t
 *  @brief      ping
 */
typedef struct {
    uint16_t    num_pong_bytes;                     ///< 2: num_pong_bytes
    uint16_t    byteslen;                           ///< 2: byteslen
} ln_ping_t;


/** @struct     ln_pong_t
 *  @brief      pong
 *  @note
 *      - byteslenはpingのnum_pong_bytesと同じにする
 */
typedef struct {
    uint16_t    byteslen;                           ///< 2: byteslen
} ln_pong_t;

/// @}


/**************************************************************************
 * typedefs : Announcement
 **************************************************************************/

/// @addtogroup announcement
/// @{

/** @struct     ln_cnl_announce_t
 *  @brief      channel_announcement
 */
typedef struct {
//    uint8_t     *p_node_signature1;                 ///< 64: node_signature_1
//    uint8_t     *p_node_signature2;                 ///< 64: node_signature_2
//    uint8_t     *p_btc_signature1;                  ///< 64: bitcoin_signature_1
//    uint8_t     *p_btc_signature2;                  ///< 64: bitcoin_signature_2
    uint64_t    short_channel_id;                   ///< 8:  short_channel_id
//    uint8_t     *p_node_id1;                        ///< 33: node_id_1
//    uint8_t     *p_node_id2;                        ///< 33: node_id_2
//    uint8_t     *p_btc_key1;                        ///< 33: bitcoin_key_1
//    uint8_t     *p_btc_key2;                        ///< 33: bitcoin_key_2
//    uint8_t     features;                           ///< 1:  features

    const ucoin_util_keys_t *p_my_node;
    const ucoin_util_keys_t *p_my_funding;
    const uint8_t           *p_peer_node_pub;
    const uint8_t           *p_peer_funding_pub;
    uint8_t                 *p_peer_node_sign;
    uint8_t                 *p_peer_btc_sign;
    ucoin_keys_sort_t       sort;                   ///< peerのln_node_announce_t.sort
} ln_cnl_announce_t;


/** @struct     ln_node_announce_t
 *  @brief      node_announcement
 */
typedef struct {
//    uint8_t             *p_signature;               ///< 64: signature
    uint32_t            timestamp;                  ///< 4:  timestamp
    uint8_t             *p_node_id;                 ///< 33: node_id
    char                *p_alias;                   ///< 32: alias
//    uint8_t     features;                           ///< 1:  features
//    uint8_t     ipaddr[4];                          ///< TODO: IPv4
//    uint16_t    port;                               ///< TODO: port

    //create
    const ucoin_util_keys_t *p_my_node;

    //受信したデータ用
    ucoin_keys_sort_t   sort;                       ///< 自ノードとのソート結果(ASC=自ノードが先)
} ln_node_announce_t;


/** @struct     ln_cnl_update_t
 *  @brief      channel_update
 */
typedef struct {
    uint8_t     *p_signature;                       ///< 64: signature
    uint64_t    short_channel_id;                   ///< 8:  short_channel_id
    uint32_t    timestamp;                          ///< 4:  timestamp
    uint16_t    flags;                              ///< 2:  flags
    uint16_t    cltv_expiry_delta;                  ///< 2:  cltv_expiry_delta
    uint64_t    htlc_minimum_msat;                  ///< 8:  htlc_minimum_msat
    uint32_t    fee_base_msat;                      ///< 4:  fee_base_msat
    uint32_t    fee_prop_millionths;                ///< 4:  fee_proportional_millionths
} ln_cnl_update_t;


/** @struct     ln_announce_signs_t
 *  @brief      announcement_signatures
 */
typedef struct {
    uint8_t     *p_channel_id;                      ///< 32: channel-id
    uint64_t    short_channel_id;                   ///< 8:  short_channel_id
    uint8_t     *p_node_signature;                  ///< 64: node_signature
    uint8_t     *p_btc_signature;                   ///< 64: bitcoin_signature
} ln_announce_signs_t;

/// @}


/**************************************************************************
 * typedefs : onion
 **************************************************************************/

/// @addtogroup onion
/// @{

/** @struct     ln_hop_datain_t
 *  @brief      ONIONパケット生成情報
 */
typedef struct {
    uint64_t            short_channel_id;               ///< short_channel_id
    uint64_t            amt_to_forward;                 ///< update_add_htlcのamount-msat
    uint32_t            outgoing_cltv_value;            ///< update_add_htlcのcltv-expiry
    uint8_t             pubkey[UCOIN_SZ_PUBKEY];        ///< ノード公開鍵(node_id)
} ln_hop_datain_t;


/** @struct     ln_hop_dataout_t
 *  @brief      ONIONパケット解析情報
 */
typedef struct {
    bool                b_exit;                         ///< true:送金先, false:中継
    uint64_t            short_channel_id;               ///< short_channel_id
    uint64_t            amt_to_forward;                 ///< update_add_htlcのamount-msat
    uint32_t            outgoing_cltv_value;            ///< update_add_htlcのcltv-expiry
} ln_hop_dataout_t;

/// @}


/**************************************************************************
 * typedefs : コールバック用
 **************************************************************************/

/** @struct ln_cb_funding_t
 *  @brief  funding_tx安定待ち要求(#LN_CB_FUNDINGTX_WAIT) / Establish完了通知(#LN_CB_ESTABLISHED)
 */
typedef struct {
    const ucoin_tx_t        *p_tx_funding;              ///< funding_tx(open_channel送信側のみ)
    const uint8_t           *p_txid;                    ///< funding txid
    uint32_t                min_depth;                  ///< minimum_depth
} ln_cb_funding_t;


/** @struct ln_cb_add_htlc_recv_t
 *  @brief  update_add_htlc受信通知(#LN_CB_ADD_HTLC_RECV)
 */
typedef struct {
    bool                        ok;                     ///< true:アプリ層処理OK
    uint64_t                    id;                     ///< HTLC id
    const uint8_t               *p_payment_hash;        ///< self->cnl_add_htlc[idx].payment_sha256
    const ln_hop_dataout_t      *p_hop;                 ///< onion解析結果
    uint64_t                    amount_msat;            ///< self->cnl_add_htlc[idx].amount_msat
    uint32_t                    cltv_expiry;            ///< self->cnl_add_htlc[idx].cltv_expiry
    uint8_t                     *p_onion_route;         ///< 変換後onionパケット(self->cnl_add_htlc[idx].p_onion_route)
} ln_cb_add_htlc_recv_t;


/** @struct ln_cb_fulfill_htlc_recv_t
 *  @brief  update_fulfill_htlc受信通知(#LN_CB_FULFILL_HTLC_RECV)
 */
typedef struct {
    uint64_t                prev_short_channel_id;  ///< 転送元
    const uint8_t           *p_preimage;            ///< update_fulfill_htlcで受信したpreimage(スタック)
    uint64_t                id;                     ///< HTLC id
} ln_cb_fulfill_htlc_recv_t;


/** @struct ln_cb_commsig_recv_t
 *  @brief  commitment_signed受信通知(#LN_CB_COMMIT_SIG_RECV)
 *  @todo
 *      - アプリ側も似たような情報を持っているので、まとめたいが方法が見つかっていない
 */
typedef struct {
    bool                    unlocked;               ///< true:送金処理完了
} ln_cb_commsig_recv_t;


/** @struct ln_cb_htlc_changed_t
 *  @brief  revoke_and_ack受信通知(#LN_CB_HTLC_CHANGED)
 */
typedef struct {
    bool                    unlocked;               ///< true:着金処理完了
} ln_cb_htlc_changed_t;


/** @struct ln_cb_closed_t
 *  @brief  Mutual Close完了通知(#LN_CB_CLOSED)
 */
typedef struct {
    ucoin_buf_t             *p_buf_bolt;            ///< peerに送信するメッセージ
    ucoin_buf_t             *p_tx_closing;          ///< ブロックチェーンに公開するtx
} ln_cb_closed_t;


/** @struct ln_cb_node_anno_recv_t
 *  @brief  node_announcement受信通知(#LN_CB_NODE_ANNO_RECV)
 */
typedef struct {
    const uint8_t           *p_node_id;             ///< 通知元node_id
    uint64_t                short_channel_id;       ///< 自分とのshort_channel_id(無ければ0)
} ln_cb_node_anno_recv_t;


/**************************************************************************
 * typedefs : 管理データ
 **************************************************************************/

/// @addtogroup channel_mng
/// @{


/** @struct ln_channel_info_t
 *  @brief  announceチャネル情報
 *  @todo
 *      - channel_announcementに耐えられるようにすべきだが、まだ至っていない
 */
typedef struct {
    int8_t                  node1;                              ///< (自ノード:NODE_MYSELF)
    int8_t                  node2;                              ///< (自ノード:NODE_MYSELF)
    uint64_t                short_channel_id;                   ///< short_channel_id
} ln_channel_info_t;


/** @struct ln_node_info_t
 *  @brief  announceノード情報
 *  @todo
 *      - channel_announcementに耐えられるようにすべきだが、まだ至っていない
 */
typedef struct {
    uint8_t                 node_id[UCOIN_SZ_PUBKEY];           ///< ノードID
    char                    alias[LN_SZ_ALIAS];                 ///< 名前
    uint32_t                timestamp;                          ///< 前回受信したtimestamp
    ucoin_keys_sort_t       sort;                               ///< 自ノードとのソート
} ln_node_info_t;


/** @struct ln_node_t
 *  @brief  ノード情報
 */
typedef struct {
    ucoin_util_keys_t           keys;                           ///< node鍵
    uint8_t                     features;                       ///< localfeatures
    char                        alias[LN_SZ_ALIAS];             ///< ノード名(\0 terminate)

    uint8_t                     node_num;                       ///< 保持しているnodes数
    ln_node_info_t              node_info[LN_NODE_MAX];

    uint8_t                     channel_num;
    ln_channel_info_t           channel_info[LN_CHANNEL_MAX];
} ln_node_t;


/** @struct ln_funding_local_data_t
 *  @brief  自ノードfunding情報
 */
typedef struct {
    uint8_t             funding_txid[UCOIN_SZ_TXID];            ///< funding-tx TXID
    uint16_t            funding_txindex;                        ///< funding-tx index

    //MSG_FUNDIDX_xxx
    ucoin_util_keys_t   keys[LN_FUNDIDX_MAX];
    //MSG_SCRIPTIDX_xxx
    ucoin_util_keys_t   scriptkeys[LN_SCRIPTIDX_MAX];
} ln_funding_local_data_t;


/** @struct ln_funding_remote_data_t
 *  @brief  他ノードfunding情報
 */
typedef struct {
    //MSG_FUNDIDX_xxx
    uint8_t             pubkeys[LN_FUNDIDX_MAX][UCOIN_SZ_PUBKEY];   ///< 相手から受信した公開鍵
    //MSG_SCRIPTIDX_xxx
    uint8_t             scriptpubkeys[LN_SCRIPTIDX_MAX][UCOIN_SZ_PUBKEY];   ///< scriptPubKey
} ln_funding_remote_data_t;


/** @struct ln_commit_data_t
 *  @brief  commitment transaction用情報
 */
typedef struct {
    uint32_t            accept_htlcs;                   ///< accept_htlcs
    uint32_t            to_self_delay;                  ///< to_self_delay
    uint64_t            minimum_msat;                   ///< minimum_msat
    uint64_t            in_flight_msat;                 ///< in_flight_msat
    uint64_t            dust_limit_sat;                 ///< dust_limit_sat
    uint8_t             signature[LN_SZ_SIGNATURE];     ///< 署名
                                                        // localには相手に送信する署名
                                                        // remoteには相手から受信した署名
} ln_commit_data_t;


/** @struct ln_noise_t
 *  @brief  BOLT#8 protocol
 */
typedef struct {
    uint8_t             sk[UCOIN_SZ_PRIVKEY];           ///< send key
    uint64_t            sn;                             ///< send nonce
    uint8_t             rk[UCOIN_SZ_PRIVKEY];           ///< receive key
    uint64_t            rn;                             ///< receive nonce
    uint8_t             ck[UCOIN_SZ_SHA256];            ///< ck

    void                *p_handshake;
} ln_noise_t;


/** @struct     ln_self_t
 *  @brief      チャネル情報
 */
struct ln_self_t {
    ln_node_t                   *p_node;                        ///< 属しているnode情報
    int8_t                      node_idx;                       ///< 接続先ノード(p_node->nodes[node_idx])
    ucoin_buf_t                 cnl_anno;                       ///< 自channel_announcement

    uint64_t                    storage_index;                  ///< 現在のindex
    uint8_t                     storage_seed[UCOIN_SZ_PRIVKEY]; ///< ユーザから指定されたseed
    ln_derkey_storage           peer_storage;                   ///< key storage(peer)
    uint64_t                    peer_storage_index;             ///< 現在のindex(peer)

    //funding
    ln_funding_local_data_t     funding_local;                  ///< funding情報:local
    ln_funding_remote_data_t    funding_remote;                 ///< funding情報:remote
    uint64_t                    obscured;                       ///< commitment numberをXORするとobscured commitment numberになる値。
    ucoin_buf_t                 redeem_fund;                    ///< 2-of-2のredeemScript
    ucoin_keys_sort_t           key_fund_sort;                  ///< 2-of-2のソート順(local, remoteを正順とした場合)
    ucoin_tx_t                  tx_funding;                     ///< funding_tx

    //closing
    ucoin_tx_t                  tx_closing;                     ///< closing_tx

    //
    ln_callback_t               p_callback;                     ///< 通知コールバック

    //msg:init
    bool                        init_sent;                      ///< true:initメッセージ送信済み
    uint8_t                     lfeature_remote;                ///< initで取得したlocalfeature
    //msg:establish
    ln_establish_t              *p_est;                         ///< Establish時ワーク領域
    //msg:close
    uint8_t                     shutdown_flag;                  ///< shutdownフラグ(SHUTDOWN_FLAG_xxx)。 b1:受信済み b0:送信済み
    uint64_t                    close_fee_sat;                  ///< closing_txのFEE
    ucoin_buf_t                 shutdown_scriptpk_local;        ///< mutual close時の送金先(local)
    ucoin_buf_t                 shutdown_scriptpk_remote;       ///< mutual close時の送金先(remote)
    ln_shutdown_t               cnl_shutdown;                   ///< 受信したshutdown
    ln_closing_signed_t         cnl_closing_signed;             ///< 受信したclosing_signed
    //msg:normal operation
    uint8_t                     htlc_changed;                   ///< HTLC変化フラグ
                                                                //      fulfill_updateの送受信でフラグを分ける
                                                                //      受信した場合、そのままcommitment_signedを送信し、revoke_and_ack送信で完了する
                                                                //      送信した場合、commitment_signed受信によってcommitment_signedを送信し、revoke_and_ack受信で完了
    uint16_t                    htlc_num;                       ///< HTLC数
    uint64_t                    commit_num;                     ///< commitment txを作るたびにインクリメントする48bitカウンタ(0～)
    uint64_t                    htlc_id_num;                    ///< update_add_htlcで使うidの管理
    uint64_t                    our_msat;                       ///< 自分の持ち分
    uint64_t                    their_msat;                     ///< 相手の持ち分
    ln_update_add_htlc_t        cnl_add_htlc[LN_HTLC_MAX];      ///< 追加したHTLC
    uint8_t                     channel_id[LN_SZ_CHANNEL_ID];   ///< channel_id
    uint64_t                    short_channel_id;               ///< short_channel_id

    uint16_t                    missing_pong_cnt;               ///< ping送信に対してpongを受信しなかった回数
    uint16_t                    last_num_pong_bytes;            ///< 最後にping送信したlast_num_pong_bytes

    //commitment transaction情報(local/remote)
    ln_commit_data_t            commit_local;                   ///< local commit_tx用
    ln_commit_data_t            commit_remote;                  ///< remote commit_tx用
    //commitment transaction情報(固有)
    uint64_t                    funding_sat;                    ///< funding_msat
    uint32_t                    feerate_per_kw;                 ///< feerate_per_kw

    ln_noise_t                  noise;                          ///< noise protocol

    //param
    void                        *p_param;                       ///< ユーザ用
};

/// @}


/**************************************************************************
 * prototypes
 **************************************************************************/

/** 初期化
 *
 * 鍵関係を、ストレージを含めて初期化している。
 *
 * @param[in,out]       self            channel情報
 * @param[in]           node            関連付けるnode
 * @param[in]           pSeed           per-commit-secret生成用
 * @param[in]           pFunc           通知用コールバック関数
 * @retval      true    成功
 */
bool ln_init(ln_self_t *self, ln_node_t *node, const uint8_t *pSeed, ln_callback_t pFunc);


/** 終了
 *
 * @param[in,out]       self            channel情報
 */
void ln_term(ln_self_t *self);


/** Channel Establish設定
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pEstablish      ワーク領域
 * @param[in]           pNodeId         Establish先(NULL可)
 * @retval      true    成功
 * @note
 *      - pEstablishは接続完了まで保持すること
 */
bool ln_set_establish(ln_self_t *self, ln_establish_t *pEstablish, const uint8_t *pNodeId);


/** funding鍵設定
 *
 * @param[in,out]       self        channel情報
 * @param[in]           pWif        funding鍵
 * @retval      true    成功
 * @attention
 *      - コールバックで #LN_CB_FINDINGWIF_REQ が要求された場合のみ呼び出すこと
 */
bool ln_set_funding_wif(ln_self_t *self, const char *pWif);


/** funding_tx情報設定
 *
 * @param[in,out]       self        channel情報
 * @param[in]           Height      funding_txが入ったブロック height
 * @param[in]           Index       funding_txのTXIDが入っているindex
 * @note
 *      - #LN_CB_FUNDINGTX_WAIT でコールバックされた後、安定後に呼び出すこと
 */
void ln_set_funding_info(ln_self_t *self, uint32_t Height, uint32_t Index);


/** shutdown時の出力先設定(pubkey)
 *
 * @param[in,out]       self            channel情報
 * @param[in]           pShutdownPubkey shutdown時のscriptPubKey用公開鍵
 * @param[in]           ShutdownPref    pShutdownPubkey用(UCOIN_PREF_P2PKH or UCOIN_PREF_NATIVE)
 * @retval      true    成功
 * @note
 *      - #ln_set_shutdown_vout_pubkey()か #ln_set_shutdown_vout_addr()のどちらかを設定する。
 */
bool ln_set_shutdown_vout_pubkey(ln_self_t *self, const uint8_t *pShutdownPubkey, int ShutdownPref);


/** shutdown時の出力先設定(address)
 *
 * @param[in,out]       self            channel情報
 * @param[in]           pAddr           shutdown時のアドレス
 * @retval      true    成功
 * @note
 *      - #ln_set_shutdown_vout_pubkey()か #ln_set_shutdown_vout_addr()のどちらかを設定する。
 */
bool ln_set_shutdown_vout_addr(ln_self_t *self, const char *pAddr);


/** noise handshake開始
 *
 * @param[in,out]       self        channel情報
 * @param[out]          pBuf        送信データ
 * @param[in]           pNodeId     送信側:接続先ノードID, 受信側:NULL
 * @retval      true    成功
 */
bool ln_handshake_start(ln_self_t *self, ucoin_buf_t *pBuf, const uint8_t *pNodeId);


/** noise handshake受信
 *
 * @param[in,out]       self        channel情報
 * @param[out]          pCont       true:次も受信を継続する(戻り値がtrue時のみ有効)
 * @param[in,out]       pBuf        in:受信データ, out:送信データ
 * @param[in]           pNodeId     送信側:接続先ノードID, 受信側:NULL
 * @retval      true    成功
 */
bool ln_handshake_recv(ln_self_t *self, bool *pCont, ucoin_buf_t *pBuf, const uint8_t *pNodeId);


bool ln_noise_enc(ln_self_t *self, ucoin_buf_t *pBuf);

uint16_t ln_noise_dec_len(ln_self_t *self, const uint8_t *pData, uint16_t Len);

bool ln_noise_dec_msg(ln_self_t *self, ucoin_buf_t *pBuf);


/** Lightningメッセージ受信処理
 *
 * @param[in,out]       self        channel情報
 * @param[out]          pBuf        処理成功時に送信するメッセージ
 * @param[in]           pData       受信データ
 * @param[in,out]       pLen        [in]pDataサイズ, [out]処理後の残りサイズ
 * @retval      true    解析成功
 * @note
 *      - accept_channel受信時、funding_txを展開し、安定するまで待ち時間が生じる。<br/>
 *          安定した後は #ln_funding_tx_stabled() を呼び出してシーケンスを継続すること。
 *
 */
bool ln_recv(ln_self_t *self, ucoin_buf_t *pBuf, const uint8_t *pData, uint16_t *pLen);


/** initメッセージ作成
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pInit           initメッセージ
 * retval       true    成功
 */
bool ln_create_init(ln_self_t *self, ucoin_buf_t *pInit);


/** open_channelメッセージ作成
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pOpen           生成したopen_channelメッセージ
 * @param[in]           pFundin         fund-in情報
 * @param[in]           FundingSat      fundingするamount[satoshi]
 * @param[in]           PushSat         push_msatするamount[satoshi]
 * retval       true    成功
 */
bool ln_create_open_channel(ln_self_t *self, ucoin_buf_t *pOpen,
            const ln_fundin_t *pFundin, uint64_t FundingSat, uint64_t PushSat);


/** funding_tx安定後の処理継続
 *
 * @param[in,out]       self                channel情報
 * @param[out]          pFundingLocked      送信データ(funding_locked)
 * @retval      ture    成功
 * @note
 *      - funding_txを展開して、confirmationがaccept_channel.min-depth以上経過したら呼び出す。
 */
bool ln_funding_tx_stabled(ln_self_t *self, ucoin_buf_t *pFundingLocked);


/** node_announcement作成
 *
 * @param[out]          node            ノード情報
 * @param[out]          pBuf            生成したnode_announcementメッセージ
 * @param[in]           TimeStamp       タイムスタンプ
 * @retval      ture    成功
 * @note
 *      - TimeStampは、相手のノードが特定できないのでUTCか？
 */
bool ln_create_node_announce(ln_node_t *node, ucoin_buf_t *pBuf, uint32_t TimeStamp);


/** announcement_signatures作成およびchannel_announcementの一部(peer署名無し)生成
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pBufAnnoSigns   生成したannouncement_signaturesメッセージ
 * @retval      ture    成功
 * @note
 *      - チャネルのどちらかでもinitのlocalfeaturesでchannels_publicを持っていない場合は失敗する。
 *      - Establish完了以降に呼び出すこと。
 */
bool ln_create_announce_signs(ln_self_t *self, ucoin_buf_t *pBufAnnoSigns);


/** closing transactionのFEE設定
 *
 * @param[in,out]       self            channel情報
 * @param[in]           Fee             FEE
 */
void ln_update_shutdown_fee(ln_self_t *self, uint64_t Fee);


/** shutdownメッセージ作成
 *
 * @param[in,out]       self        channel情報
 * @param[out]          pShutdown   生成したshutdownメッセージ
 * @retval      ture    成功
 * @note
 *      - scriptPubKeyは #ln_init()で指定したアドレスを使用する
 */
bool ln_create_shutdown(ln_self_t *self, ucoin_buf_t *pShutdown);


/** update_add_htlcメッセージ作成
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pAdd            生成したupdate_add_htlcメッセージ
 * @param[in]           pPacket         onion packet
 * @param[in]           amount_msat     送金額[msat]
 * @param[in]           cltv_value      CLTV値
 * @param[in]           pPaymentHash    PaymentHash(SHA256:32byte)
 * @param[in]           prev_short_channel_id   転送元short_channel_id(ない場合は0)
 * @retval      true    成功
 * @note
 *      - prev_short_channel_id はfullfillの通知先として使用する
 */
bool ln_create_add_htlc(ln_self_t *self, ucoin_buf_t *pAdd,
            const uint8_t *pPacket,
            uint64_t amount_msat,
            uint32_t cltv_value,
            const uint8_t *pPaymentHash,
            uint64_t prev_short_channel_id);


/** update_fulfill_htlcメッセージ作成
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pFulfill        生成したupdate_fulfill_htlcメッセージ
 * @param[in]           id              HTLC id
 * @param[in]           pPreImage       反映するHTLCのpayment-preimage
 * @retval      true    成功
 */
bool ln_create_fulfill_htlc(ln_self_t *self, ucoin_buf_t *pFulfill, uint64_t id, const uint8_t *pPreImage);


/** commitment_signature作成
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pCommSig        生成したcommitment_signatureメッセージ
 * @retval      true    成功
 */
bool ln_create_commit_signed(ln_self_t *self, ucoin_buf_t *pCommSig);


/** ping作成
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pPing           生成したpingメッセージ
 * @retval      true    成功
 */
bool ln_create_ping(ln_self_t *self, ucoin_buf_t *pPing);


/** pong作成
 *
 * @param[in,out]       self            channel情報
 * @param[out]          pPong           生成したpongメッセージ
 * @param[in]           NumPongBytes    pingのnum_pong_bytes
 * @retval      true    成功
 */
bool ln_create_pong(ln_self_t *self, ucoin_buf_t *pPong, uint16_t NumPongBytes);


/** PreImageハッシュ計算
 *
 * @param[out]      pHash               計算結果(LN_SZ_HASH)
 * @param[in]       pPreImage           計算元(LN_SZ_PREIMAGE)
 */
void ln_calc_preimage_hash(uint8_t *pHash, const uint8_t *pPreImage);


/********************************************************************
 * NODE
 ********************************************************************/

/** ノード情報初期化
 *
 * @param[out]      node            ノード情報
 * @param[in]       pWif            ノード秘密鍵
 * @param[in]       pNodeName       ノード名
 * @param[in]       Features        ?
 */
void ln_node_init(ln_node_t *node, const char *pWif, const char *pNodeName, uint8_t Features);


/** ノード情報終了
 *
 * @param[in,out]   node            ノード情報
 */
void ln_node_term(ln_node_t *node);


/** short_channel_id検索
 *
 * @param[in,out]   node            ノード情報
 * @param[in]       p_node_id       検索するnode_id
 * @retval          0以外       検索したshort_channel_id
 * @retval          0           検索失敗
 */
uint64_t ln_node_search_short_cnl_id(ln_node_t *node, const uint8_t *p_node_id);


/********************************************************************
 * ONION
 ********************************************************************/

/** ONIONパケット生成
 *
 * @param[in,out]   self                channel情報
 * @param[out]      pPacket             ONIONパケット[LN_SZ_ONION_ROUTE]
 * @param[in]       pHopData            HOPデータ
 * @param[in]       NumHops             pHopData数
 * @param[in]       pSessionKey         セッション鍵[UCOIN_SZ_PRIVKEY]
 * @param[in]       pAssocData          Associated Data
 * @param[in]       AssocLen            pAssocData長
 * @retval      true    成功
 */
bool ln_onion_create_packet(ln_self_t *self,
            uint8_t *pPacket,
            const ln_hop_datain_t *pHopData,
            int NumHops,
            const uint8_t *pSessionKey,
            const uint8_t *pAssocData, int AssocLen);


/********************************************************************
 * デバッグ
 ********************************************************************/

#ifdef UCOIN_USE_PRINTFUNC
void ln_print_self(const ln_self_t *self);
void ln_print_node(const ln_node_t *node);
#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* LN_H__ */