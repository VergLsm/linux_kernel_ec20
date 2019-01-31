/*
 * Linux cfg80211 driver - Android related functions
 *
 * $ Copyright Open Broadcom Corporation $
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: wl_android.c 674374 2017-10-20 07:38:00Z $
 */

#include <linux/module.h>
#include <linux/netdevice.h>
#include <net/netlink.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include <wl_android.h>
#include <wldev_common.h>
#include <wlioctl.h>
#include <wlioctl_utils.h>
#include <bcmutils.h>
#include <linux_osl.h>
#include <dhd_dbg.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <proto/bcmip.h>
#ifdef PNO_SUPPORT
#include <dhd_pno.h>
#endif
#include <bcmsdbus.h>
#ifdef WL_CFG80211
#include <wl_cfg80211.h>
#endif
#ifdef WL_NAN
#include <wl_cfgnan.h>
#endif /* WL_NAN */
#ifdef DHDTCPACK_SUPPRESS
#include <dhd_ip.h>
#endif /* DHDTCPACK_SUPPRESS */

/*
 * Android private command strings, PLEASE define new private commands here
 * so they can be updated easily in the future (if needed)
 */

#define CMD_START		"START"
#define CMD_STOP		"STOP"
#define	CMD_SCAN_ACTIVE		"SCAN-ACTIVE"
#define	CMD_SCAN_PASSIVE	"SCAN-PASSIVE"
#define CMD_RSSI		"RSSI"
#define CMD_LINKSPEED		"LINKSPEED"
#define CMD_RXFILTER_START	"RXFILTER-START"
#define CMD_RXFILTER_STOP	"RXFILTER-STOP"
#define CMD_RXFILTER_ADD	"RXFILTER-ADD"
#define CMD_RXFILTER_REMOVE	"RXFILTER-REMOVE"
#define CMD_BTCOEXSCAN_START	"BTCOEXSCAN-START"
#define CMD_BTCOEXSCAN_STOP	"BTCOEXSCAN-STOP"
#define CMD_BTCOEXMODE		"BTCOEXMODE"
#define CMD_SETSUSPENDOPT	"SETSUSPENDOPT"
#define CMD_SETSUSPENDMODE      "SETSUSPENDMODE"
#define CMD_P2P_DEV_ADDR	"P2P_DEV_ADDR"
#define CMD_SETFWPATH		"SETFWPATH"
#define CMD_SETBAND		"SETBAND"
#define CMD_GETBAND		"GETBAND"
#define CMD_COUNTRY		"COUNTRY"
#define CMD_P2P_SET_NOA		"P2P_SET_NOA"
#if !defined WL_ENABLE_P2P_IF
#define CMD_P2P_GET_NOA			"P2P_GET_NOA"
#endif /* WL_ENABLE_P2P_IF */
#define CMD_P2P_SD_OFFLOAD		"P2P_SD_"
#define CMD_P2P_SET_PS		"P2P_SET_PS"
#define CMD_P2P_ECSA		"P2P_ECSA"
#define CMD_SET_AP_WPS_P2P_IE 		"SET_AP_WPS_P2P_IE"
#define CMD_SETROAMMODE 	"SETROAMMODE"
#define CMD_SETIBSSBEACONOUIDATA	"SETIBSSBEACONOUIDATA"
#define CMD_MIRACAST		"MIRACAST"
#ifdef WL_NAN
#define CMD_NAN         "NAN_"
#endif /* WL_NAN */
#define CMD_COUNTRY_DELIMITER "/"
#ifdef WL11ULB
#define CMD_ULB_MODE "ULB_MODE"
#define CMD_ULB_BW "ULB_BW"
#endif /* WL11ULB */

#if defined(WL_SUPPORT_AUTO_CHANNEL)
#define CMD_GET_BEST_CHANNELS	"GET_BEST_CHANNELS"
#endif /* WL_SUPPORT_AUTO_CHANNEL */

#define CMD_80211_MODE    "MODE"  /* 802.11 mode a/b/g/n/ac */
#define CMD_CHANSPEC      "CHANSPEC"
#define CMD_DATARATE      "DATARATE"
#define CMD_ASSOC_CLIENTS "ASSOCLIST"
#define CMD_SET_CSA       "SETCSA"
#ifdef WL_SUPPORT_AUTO_CHANNEL
#define CMD_SET_HAPD_AUTO_CHANNEL	"HAPD_AUTO_CHANNEL"
#endif /* WL_SUPPORT_AUTO_CHANNEL */
#ifdef CUSTOMER_HW4
#ifdef SUPPORT_HIDDEN_AP
/* Hostapd private command */
#define CMD_SET_HAPD_MAX_NUM_STA	"HAPD_MAX_NUM_STA"
#define CMD_SET_HAPD_SSID		"HAPD_SSID"
#define CMD_SET_HAPD_HIDE_SSID		"HAPD_HIDE_SSID"
#endif /* SUPPORT_HIDDEN_AP */
#ifdef SUPPORT_SOFTAP_SINGL_DISASSOC
#define CMD_HAPD_STA_DISASSOC		"HAPD_STA_DISASSOC"
#endif /* SUPPORT_SOFTAP_SINGL_DISASSOC */
#ifdef SUPPORT_SET_LPC
#define CMD_HAPD_LPC_ENABLED		"HAPD_LPC_ENABLED"
#endif /* SUPPORT_SET_LPC */
#ifdef SUPPORT_TRIGGER_HANG_EVENT
#define CMD_TEST_FORCE_HANG		"TEST_FORCE_HANG"
#endif /* SUPPORT_TRIGGER_HANG_EVENT */
#ifdef SUPPORT_LTECX
#define CMD_LTECX_SET		"LTECOEX"
#endif /* SUPPORT_LTECX */
#ifdef TEST_TX_POWER_CONTROL
#define CMD_TEST_SET_TX_POWER		"TEST_SET_TX_POWER"
#define CMD_TEST_GET_TX_POWER		"TEST_GET_TX_POWER"
#endif /* TEST_TX_POWER_CONTROL */
#define CMD_SARLIMIT_TX_CONTROL		"SET_TX_POWER_CALLING"
#endif /* CUSTOMER_HW4 */
#define CMD_KEEP_ALIVE		"KEEPALIVE"

/* CCX Private Commands */
#ifdef BCMCCX
#define CMD_GETCCKM_RN		"get cckm_rn"
#define CMD_SETCCKM_KRK		"set cckm_krk"
#define CMD_GET_ASSOC_RES_IES	"get assoc_res_ies"
#endif

#ifdef PNO_SUPPORT
#define CMD_PNOSSIDCLR_SET	"PNOSSIDCLR"
#define CMD_PNOSETUP_SET	"PNOSETUP "
#define CMD_PNOENABLE_SET	"PNOFORCE"
#define CMD_PNODEBUG_SET	"PNODEBUG"
#define CMD_WLS_BATCHING	"WLS_BATCHING"
#endif /* PNO_SUPPORT */

#define CMD_OKC_SET_PMK		"SET_PMK"
#define CMD_OKC_ENABLE		"OKC_ENABLE"

#define	CMD_HAPD_MAC_FILTER	"HAPD_MAC_FILTER"

#ifdef CUSTOMER_HW4

#ifdef ROAM_API
#define CMD_ROAMTRIGGER_SET "SETROAMTRIGGER"
#define CMD_ROAMTRIGGER_GET "GETROAMTRIGGER"
#define CMD_ROAMDELTA_SET "SETROAMDELTA"
#define CMD_ROAMDELTA_GET "GETROAMDELTA"
#define CMD_ROAMSCANPERIOD_SET "SETROAMSCANPERIOD"
#define CMD_ROAMSCANPERIOD_GET "GETROAMSCANPERIOD"
#define CMD_FULLROAMSCANPERIOD_SET "SETFULLROAMSCANPERIOD"
#define CMD_FULLROAMSCANPERIOD_GET "GETFULLROAMSCANPERIOD"
#define CMD_COUNTRYREV_SET "SETCOUNTRYREV"
#define CMD_COUNTRYREV_GET "GETCOUNTRYREV"
#endif /* ROAM_API */

#ifdef WES_SUPPORT
#define CMD_GETROAMSCANCONTROL "GETROAMSCANCONTROL"
#define CMD_SETROAMSCANCONTROL "SETROAMSCANCONTROL"
#define CMD_GETROAMSCANCHANNELS "GETROAMSCANCHANNELS"
#define CMD_SETROAMSCANCHANNELS "SETROAMSCANCHANNELS"

#define CMD_GETSCANCHANNELTIME "GETSCANCHANNELTIME"
#define CMD_SETSCANCHANNELTIME "SETSCANCHANNELTIME"
#define CMD_GETSCANHOMETIME "GETSCANHOMETIME"
#define CMD_SETSCANHOMETIME "SETSCANHOMETIME"
#define CMD_GETSCANHOMEAWAYTIME "GETSCANHOMEAWAYTIME"
#define CMD_SETSCANHOMEAWAYTIME "SETSCANHOMEAWAYTIME"
#define CMD_GETSCANNPROBES "GETSCANNPROBES"
#define CMD_SETSCANNPROBES "SETSCANNPROBES"
#define CMD_GETDFSSCANMODE "GETDFSSCANMODE"
#define CMD_SETDFSSCANMODE "SETDFSSCANMODE"
#define CMD_SETJOINPREFER "SETJOINPREFER"

#define CMD_SENDACTIONFRAME "SENDACTIONFRAME"
#define CMD_REASSOC "REASSOC"

#define CMD_GETWESMODE "GETWESMODE"
#define CMD_SETWESMODE "SETWESMODE"

#define CMD_GETOKCMODE "GETOKCMODE"
#define CMD_SETOKCMODE "SETOKCMODE"

#define ANDROID_WIFI_MAX_ROAM_SCAN_CHANNELS 100

typedef struct android_wifi_reassoc_params {
	unsigned char bssid[18];
	int channel;
} android_wifi_reassoc_params_t;

#define ANDROID_WIFI_REASSOC_PARAMS_SIZE sizeof(struct android_wifi_reassoc_params)

#define ANDROID_WIFI_ACTION_FRAME_SIZE 1040

typedef struct android_wifi_af_params {
	unsigned char bssid[18];
	int channel;
	int dwell_time;
	int len;
	unsigned char data[ANDROID_WIFI_ACTION_FRAME_SIZE];
} android_wifi_af_params_t;

#define ANDROID_WIFI_AF_PARAMS_SIZE sizeof(struct android_wifi_af_params)
#endif /* WES_SUPPORT */
#ifdef SUPPORT_AMPDU_MPDU_CMD
#define CMD_AMPDU_MPDU		"AMPDU_MPDU"
#endif /* SUPPORT_AMPDU_MPDU_CMD */

#define CMD_CHANGE_RL 	"CHANGE_RL"
#define CMD_RESTORE_RL  "RESTORE_RL"

#define CMD_SET_RMC_ENABLE			"SETRMCENABLE"
#define CMD_SET_RMC_TXRATE			"SETRMCTXRATE"
#define CMD_SET_RMC_ACTPERIOD		"SETRMCACTIONPERIOD"
#define CMD_SET_RMC_IDLEPERIOD		"SETRMCIDLEPERIOD"
#define CMD_SET_RMC_LEADER			"SETRMCLEADER"
#define CMD_SET_RMC_EVENT			"SETRMCEVENT"

#define CMD_SET_SCSCAN		"SETSINGLEANT"
#define CMD_GET_SCSCAN		"GETSINGLEANT"

#endif /* CUSTOMER_HW4 */
#ifdef WLFBT
#define CMD_GET_FTKEY      "GET_FTKEY"
#endif

#ifdef WLAIBSS
#define CMD_SETIBSSTXFAILEVENT		"SETIBSSTXFAILEVENT"
#define CMD_GET_IBSS_PEER_INFO		"GETIBSSPEERINFO"
#define CMD_GET_IBSS_PEER_INFO_ALL	"GETIBSSPEERINFOALL"
#define CMD_SETIBSSROUTETABLE		"SETIBSSROUTETABLE"
#define CMD_SETIBSSAMPDU			"SETIBSSAMPDU"
#define CMD_SETIBSSANTENNAMODE		"SETIBSSANTENNAMODE"
#endif /* WLAIBSS */

#define CMD_ROAM_OFFLOAD			"SETROAMOFFLOAD"
#define CMD_ROAM_OFFLOAD_APLIST			"SETROAMOFFLAPLIST"
#define CMD_INTERFACE_CREATE			"INTERFACE_CREATE"
#define CMD_INTERFACE_DELETE			"INTERFACE_DELETE"

#ifdef P2PRESP_WFDIE_SRC
#define CMD_P2P_SET_WFDIE_RESP      "P2P_SET_WFDIE_RESP"
#define CMD_P2P_GET_WFDIE_RESP      "P2P_GET_WFDIE_RESP"
#endif /* P2PRESP_WFDIE_SRC */

#define CMD_DFS_AP_MOVE			"DFS_AP_MOVE"
#define CMD_WBTEXT_ENABLE		"WBTEXT_ENABLE"
#define CMD_WBTEXT_PROFILE_CONFIG	"WBTEXT_PROFILE_CONFIG"
#define CMD_WBTEXT_WEIGHT_CONFIG	"WBTEXT_WEIGHT_CONFIG"
#define CMD_WBTEXT_TABLE_CONFIG		"WBTEXT_TABLE_CONFIG"
#define CMD_WBTEXT_DELTA_CONFIG		"WBTEXT_DELTA_CONFIG"

#ifdef WLWFDS
#define CMD_ADD_WFDS_HASH	"ADD_WFDS_HASH"
#define CMD_DEL_WFDS_HASH	"DEL_WFDS_HASH"
#endif /* WLWFDS */

#ifdef SET_RPS_CPUS
#define CMD_RPSMODE  "RPSMODE"
#endif /* SET_RPS_CPUS */

#ifdef BT_WIFI_HANDOVER
#define CMD_TBOW_TEARDOWN "TBOW_TEARDOWN"
#endif /* BT_WIFI_HANDOVER */

/* miracast related definition */
#define MIRACAST_MODE_OFF	0
#define MIRACAST_MODE_SOURCE	1
#define MIRACAST_MODE_SINK	2

#ifndef MIRACAST_AMPDU_SIZE
#define MIRACAST_AMPDU_SIZE	8
#endif

#ifndef MIRACAST_MCHAN_ALGO
#define MIRACAST_MCHAN_ALGO     1
#endif

#ifndef MIRACAST_MCHAN_BW
#define MIRACAST_MCHAN_BW       25
#endif

#ifdef CONNECTION_STATISTICS
#define CMD_GET_CONNECTION_STATS	"GET_CONNECTION_STATS"

struct connection_stats {
	u32 txframe;
	u32 txbyte;
	u32 txerror;
	u32 rxframe;
	u32 rxbyte;
	u32 txfail;
	u32 txretry;
	u32 txretrie;
	u32 txrts;
	u32 txnocts;
	u32 txexptime;
	u32 txrate;
	u8	chan_idle;
};
#endif /* CONNECTION_STATISTICS */

static LIST_HEAD(miracast_resume_list);
static u8 miracast_cur_mode;

struct io_cfg {
	s8 *iovar;
	s32 param;
	u32 ioctl;
	void *arg;
	u32 len;
	struct list_head list;
};

#if defined(BCMFW_ROAM_ENABLE)
#define CMD_SET_ROAMPREF	"SET_ROAMPREF"

#define MAX_NUM_SUITES		10
#define WIDTH_AKM_SUITE		8
#define JOIN_PREF_RSSI_LEN		0x02
#define JOIN_PREF_RSSI_SIZE		4	/* RSSI pref header size in bytes */
#define JOIN_PREF_WPA_HDR_SIZE		4 /* WPA pref header size in bytes */
#define JOIN_PREF_WPA_TUPLE_SIZE	12	/* Tuple size in bytes */
#define JOIN_PREF_MAX_WPA_TUPLES	16
#define MAX_BUF_SIZE		(JOIN_PREF_RSSI_SIZE + JOIN_PREF_WPA_HDR_SIZE +	\
				           (JOIN_PREF_WPA_TUPLE_SIZE * JOIN_PREF_MAX_WPA_TUPLES))
#endif /* BCMFW_ROAM_ENABLE */

#ifdef WL_GENL
static s32 wl_genl_handle_msg(struct sk_buff *skb, struct genl_info *info);
static int wl_genl_init(void);
static int wl_genl_deinit(void);

extern struct net init_net;
/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h
 */
static struct nla_policy wl_genl_policy[BCM_GENL_ATTR_MAX + 1] = {
	[BCM_GENL_ATTR_STRING] = { .type = NLA_NUL_STRING },
	[BCM_GENL_ATTR_MSG] = { .type = NLA_BINARY },
};


/* commands: mapping between the command enumeration and the actual function */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
struct genl_ops wl_genl_ops[] = {
	{
	.cmd = BCM_GENL_CMD_MSG,
	.flags = 0,
	.policy = wl_genl_policy,
	.doit = wl_genl_handle_msg,
	.dumpit = NULL,
	},
};
#else
struct genl_ops wl_genl_ops = {
	.cmd = BCM_GENL_CMD_MSG,
	.flags = 0,
	.policy = wl_genl_policy,
	.doit = wl_genl_handle_msg,
	.dumpit = NULL,

};
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0) */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
static struct genl_multicast_group wl_genl_mcast[] = {
	 { .name = "bcm-genl-mcast", },
};
#else
static struct genl_multicast_group wl_genl_mcast = {
	.id = GENL_ID_GENERATE,    /* Genetlink would generate the ID */
	.name = "bcm-genl-mcast",
};
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0) */

/* Move the definition here since kernel 4.10.0 needs the wl_genl_ops[] defined before it */
#define WL_GENL_VER 1
/* family definition */
static struct genl_family wl_genl_family = {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0))
	.id = GENL_ID_GENERATE,    /* Genetlink would generate the ID */
#else
	.ops = wl_genl_ops,
	.mcgrps = wl_genl_mcast,
#endif
	.hdrsize = 0,
	.name = "bcm-genl",        /* Netlink I/F for Android */
	.version = WL_GENL_VER,     /* Version Number */
	.maxattr = BCM_GENL_ATTR_MAX,
};
#endif /* WL_GENL */


/**
 * Extern function declarations (TODO: move them to dhd_linux.h)
 */
int dhd_net_bus_devreset(struct net_device *dev, uint8 flag);
int dhd_dev_init_ioctl(struct net_device *dev);
#ifdef WL_CFG80211
int wl_cfg80211_get_p2p_dev_addr(struct net_device *net, struct ether_addr *p2pdev_addr);
int wl_cfg80211_set_btcoex_dhcp(struct net_device *dev, dhd_pub_t *dhd, char *command);
#if defined(CUSTOMER_HW4) && defined(WES_SUPPORT)
int wl_cfg80211_set_wes_mode(int mode);
int wl_cfg80211_get_wes_mode(void);
#endif
#else
int wl_cfg80211_get_p2p_dev_addr(struct net_device *net, struct ether_addr *p2pdev_addr)
{ return 0; }
int wl_cfg80211_set_p2p_noa(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_get_p2p_noa(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_set_p2p_ps(struct net_device *net, char* buf, int len)
{ return 0; }
int wl_cfg80211_set_p2p_ecsa(struct net_device *net, char* buf, int len)
{ return 0; }
#endif /* WK_CFG80211 */

#if defined(CUSTOMER_HW4) && defined(WES_SUPPORT)
/* wl_roam.c */
extern int get_roamscan_mode(struct net_device *dev, int *mode);
extern int set_roamscan_mode(struct net_device *dev, int mode);
extern int get_roamscan_channel_list(struct net_device *dev, unsigned char channels[]);
extern int set_roamscan_channel_list(struct net_device *dev, unsigned char n,
	unsigned char channels[], int ioctl_ver);
#endif
#if defined(CUSTOMER_HW4) && defined(ROAM_CHANNEL_CACHE)
extern void wl_update_roamscan_cache_by_band(struct net_device *dev, int band);
#endif

#ifdef ENABLE_4335BT_WAR
extern int bcm_bt_lock(int cookie);
extern void bcm_bt_unlock(int cookie);
static int lock_cookie_wifi = 'W' | 'i'<<8 | 'F'<<16 | 'i'<<24;	/* cookie is "WiFi" */
#endif /* ENABLE_4335BT_WAR */

extern bool ap_fw_loaded;
extern char iface_name[IFNAMSIZ];

/**
 * Local (static) functions and variables
 */

/* Initialize g_wifi_on to 1 so dhd_bus_start will be called for the first
 * time (only) in dhd_open, subsequential wifi on will be handled by
 * wl_android_wifi_on
 */
static int g_wifi_on = TRUE;

/**
 * Local (static) function definitions
 */

#ifdef WLWFDS
static int wl_android_set_wfds_hash(
	struct net_device *dev, char *command, int total_len, bool enable)
{
	int error = 0;
	wl_p2p_wfds_hash_t *wfds_hash = NULL;
	char *smbuf = NULL;
	smbuf = kmalloc(WLC_IOCTL_MAXLEN, GFP_KERNEL);

	if (smbuf == NULL) {
		DHD_ERROR(("%s: failed to allocated memory %d bytes\n",
			__FUNCTION__, WLC_IOCTL_MAXLEN));
		return -ENOMEM;
	}

	if (enable) {
		wfds_hash = (wl_p2p_wfds_hash_t *)(command + strlen(CMD_ADD_WFDS_HASH) + 1);
		error = wldev_iovar_setbuf(dev, "p2p_add_wfds_hash", wfds_hash,
			sizeof(wl_p2p_wfds_hash_t), smbuf, WLC_IOCTL_MAXLEN, NULL);
	}
	else {
		wfds_hash = (wl_p2p_wfds_hash_t *)(command + strlen(CMD_DEL_WFDS_HASH) + 1);
		error = wldev_iovar_setbuf(dev, "p2p_del_wfds_hash", wfds_hash,
			sizeof(wl_p2p_wfds_hash_t), smbuf, WLC_IOCTL_MAXLEN, NULL);
	}

	if (error) {
		DHD_ERROR(("%s: failed to %s, error=%d\n", __FUNCTION__, command, error));
	}

	if (smbuf)
		kfree(smbuf);
	return error;
}
#endif /* WLWFDS */

static int wl_android_get_link_speed(struct net_device *net, char *command, int total_len)
{
	int link_speed;
	int bytes_written;
	int error;

	error = wldev_get_link_speed(net, &link_speed);
	if (error) {
		DHD_ERROR(("Get linkspeed failed \n"));
		return -1;
	}

	/* Convert Kbps to Android Mbps */
	link_speed = link_speed / 1000;
	bytes_written = snprintf(command, total_len, "LinkSpeed %d", link_speed);
	DHD_INFO(("%s: command result is %s\n", __FUNCTION__, command));
	return bytes_written;
}

static int wl_android_get_rssi(struct net_device *net, char *command, int total_len)
{
	wlc_ssid_t ssid = {0};
	int bytes_written = 0;
	int error = 0;
	scb_val_t scbval;
	char *delim = NULL;

	delim = strchr(command, ' ');
	/* For Ap mode rssi command would be
	 * driver rssi <sta_mac_addr>
	 * for STA/GC mode
	 * driver rssi
	*/
	if (delim) {
		/* Ap/GO mode
		* driver rssi <sta_mac_addr>
		*/
		DHD_TRACE(("%s: cmd:%s\n", __FUNCTION__, delim));
		/* skip space from delim after finding char */
		delim++;
		if (!(bcm_ether_atoe((delim), &scbval.ea)))
		{
			DHD_ERROR(("%s:address err\n", __FUNCTION__));
			return -1;
		}
	        scbval.val = htod32(0);
		DHD_TRACE(("%s: address:"MACDBG, __FUNCTION__, MAC2STRDBG(scbval.ea.octet)));
	}
	else {
		/* STA/GC mode */
		memset(&scbval, 0, sizeof(scb_val_t));
	}

	error = wldev_get_rssi(net, &scbval);
	if (error)
		return -1;

	error = wldev_get_ssid(net, &ssid);
	if (error)
		return -1;
	if ((ssid.SSID_len == 0) || (ssid.SSID_len > DOT11_MAX_SSID_LEN)) {
		DHD_ERROR(("%s: wldev_get_ssid failed\n", __FUNCTION__));
	} else {
		memcpy(command, ssid.SSID, ssid.SSID_len);
		bytes_written = ssid.SSID_len;
	}
	bytes_written += snprintf(&command[bytes_written], total_len, " rssi %d", scbval.val);
	DHD_TRACE(("%s: command result is %s (%d)\n", __FUNCTION__, command, bytes_written));
	return bytes_written;
}

static int wl_android_set_suspendopt(struct net_device *dev, char *command, int total_len)
{
	int suspend_flag;
	int ret_now;
	int ret = 0;

#ifdef CUSTOMER_HW4
	if (!dhd_download_fw_on_driverload) {
#endif /* CUSTOMER_HW4 */
		suspend_flag = *(command + strlen(CMD_SETSUSPENDOPT) + 1) - '0';

		if (suspend_flag != 0)
			suspend_flag = 1;
		ret_now = net_os_set_suspend_disable(dev, suspend_flag);

		if (ret_now != suspend_flag) {
			if (!(ret = net_os_set_suspend(dev, ret_now, 1)))
				DHD_INFO(("%s: Suspend Flag %d -> %d\n",
					__FUNCTION__, ret_now, suspend_flag));
			else
				DHD_ERROR(("%s: failed %d\n", __FUNCTION__, ret));
		}
#ifdef CUSTOMER_HW4
	}
#endif /* CUSTOMER_HW4 */
	return ret;
}

static int wl_android_set_suspendmode(struct net_device *dev, char *command, int total_len)
{
	int ret = 0;

#if !defined(CONFIG_HAS_EARLYSUSPEND) || !defined(DHD_USE_EARLYSUSPEND)
	int suspend_flag;

	suspend_flag = *(command + strlen(CMD_SETSUSPENDMODE) + 1) - '0';
	if (suspend_flag != 0)
		suspend_flag = 1;

	if (!(ret = net_os_set_suspend(dev, suspend_flag, 0)))
		DHD_INFO(("%s: Suspend Mode %d\n", __FUNCTION__, suspend_flag));
	else
		DHD_ERROR(("%s: failed %d\n", __FUNCTION__, ret));
#endif

	return ret;
}

int wl_android_get_80211_mode(struct net_device *dev, char *command, int total_len)
{
	uint8 mode[4];
	int  error = 0;
	int bytes_written = 0;

	error = wldev_get_mode(dev, mode);
	if (error)
		return -1;

	DHD_INFO(("%s: mode:%s\n", __FUNCTION__, mode));
	bytes_written = snprintf(command, total_len, "%s %s", CMD_80211_MODE, mode);
	DHD_INFO(("%s: command:%s EXIT\n", __FUNCTION__, command));
	return bytes_written;

}

extern chanspec_t
wl_chspec_driver_to_host(chanspec_t chanspec);
int wl_android_get_chanspec(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int chsp = {0};
	uint16 band = 0;
	uint16 bw = 0;
	uint16 channel = 0;
	u32 sb = 0;
	chanspec_t chanspec;

	/* command is
	 * driver chanspec
	 */
	error = wldev_iovar_getint(dev, "chanspec", &chsp);
	if (error)
		return -1;

	chanspec = wl_chspec_driver_to_host(chsp);
	DHD_INFO(("%s:return value of chanspec:%x\n", __FUNCTION__, chanspec));

	channel = chanspec & WL_CHANSPEC_CHAN_MASK;
	band = chanspec & WL_CHANSPEC_BAND_MASK;
	bw = chanspec & WL_CHANSPEC_BW_MASK;

	DHD_INFO(("%s:channel:%d band:%d bandwidth:%d\n", __FUNCTION__, channel, band, bw));

	if (bw == WL_CHANSPEC_BW_80)
		bw = WL_CH_BANDWIDTH_80MHZ;
	else if (bw == WL_CHANSPEC_BW_40)
		bw = WL_CH_BANDWIDTH_40MHZ;
	else if	(bw == WL_CHANSPEC_BW_20)
		bw = WL_CH_BANDWIDTH_20MHZ;
	else
		bw = WL_CH_BANDWIDTH_20MHZ;

	if (bw == WL_CH_BANDWIDTH_40MHZ) {
		if (CHSPEC_SB_UPPER(chanspec)) {
			channel += CH_10MHZ_APART;
		} else {
			channel -= CH_10MHZ_APART;
		}
	}
	else if (bw == WL_CH_BANDWIDTH_80MHZ) {
		sb = chanspec & WL_CHANSPEC_CTL_SB_MASK;
		if (sb == WL_CHANSPEC_CTL_SB_LL) {
			channel -= (CH_10MHZ_APART + CH_20MHZ_APART);
		} else if (sb == WL_CHANSPEC_CTL_SB_LU) {
			channel -= CH_10MHZ_APART;
		} else if (sb == WL_CHANSPEC_CTL_SB_UL) {
			channel += CH_10MHZ_APART;
		} else {
			/* WL_CHANSPEC_CTL_SB_UU */
			channel += (CH_10MHZ_APART + CH_20MHZ_APART);
		}
	}
	bytes_written = snprintf(command, total_len, "%s channel %d band %s bw %d", CMD_CHANSPEC,
		channel, band == WL_CHANSPEC_BAND_5G ? "5G":"2G", bw);

	DHD_INFO(("%s: command:%s EXIT\n", __FUNCTION__, command));
	return bytes_written;

}

/* returns current datarate datarate returned from firmware are in 500kbps */
int wl_android_get_datarate(struct net_device *dev, char *command, int total_len)
{
	int  error = 0;
	int datarate = 0;
	int bytes_written = 0;

	error = wldev_get_datarate(dev, &datarate);
	if (error)
		return -1;

	DHD_INFO(("%s:datarate:%d\n", __FUNCTION__, datarate));

	bytes_written = snprintf(command, total_len, "%s %d", CMD_DATARATE, (datarate/2));
	return bytes_written;
}
int wl_android_get_assoclist(struct net_device *dev, char *command, int total_len)
{
	int  error = 0;
	int bytes_written = 0;
	uint i;
	char mac_buf[MAX_NUM_OF_ASSOCLIST *
		sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;

	DHD_TRACE(("%s: ENTER\n", __FUNCTION__));

	assoc_maclist->count = htod32(MAX_NUM_OF_ASSOCLIST);

	error = wldev_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist, sizeof(mac_buf), false);
	if (error)
		return -1;

	assoc_maclist->count = dtoh32(assoc_maclist->count);
	bytes_written = snprintf(command, total_len, "%s listcount: %d Stations:",
		CMD_ASSOC_CLIENTS, assoc_maclist->count);

	for (i = 0; i < assoc_maclist->count; i++) {
		bytes_written += snprintf(command + bytes_written, total_len, " " MACDBG,
			MAC2STRDBG(assoc_maclist->ea[i].octet));
	}
	return bytes_written;

}
extern chanspec_t
wl_chspec_host_to_driver(chanspec_t chanspec);
static int wl_android_set_csa(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	char smbuf[WLC_IOCTL_SMLEN];
	wl_chan_switch_t csa_arg;
	u32 chnsp = 0;
	int err = 0;

	DHD_INFO(("%s: command:%s\n", __FUNCTION__, command));

	command = (command + strlen(CMD_SET_CSA));
	/* Order is mode, count channel */
	if (!*++command) {
		DHD_ERROR(("%s:error missing arguments\n", __FUNCTION__));
		return -1;
	}
	csa_arg.mode = bcm_atoi(command);

	if (csa_arg.mode != 0 && csa_arg.mode != 1) {
		DHD_ERROR(("Invalid mode\n"));
		return -1;
	}

	if (!*++command) {
		DHD_ERROR(("%s:error missing count\n", __FUNCTION__));
		return -1;
	}
	command++;
	csa_arg.count = bcm_atoi(command);

	csa_arg.reg = 0;
	csa_arg.chspec = 0;
	command += 2;
	if (!*command) {
		DHD_ERROR(("%s:error missing channel\n", __FUNCTION__));
		return -1;
	}

	chnsp = wf_chspec_aton(command);
	if (chnsp == 0)	{
		DHD_ERROR(("%s:chsp is not correct\n", __FUNCTION__));
		return -1;
	}
	chnsp = wl_chspec_host_to_driver(chnsp);
	csa_arg.chspec = chnsp;

	if (chnsp & WL_CHANSPEC_BAND_5G) {
		u32 chanspec = chnsp;
		err = wldev_iovar_getint(dev, "per_chan_info", &chanspec);
		if (!err) {
			if ((chanspec & WL_CHAN_RADAR) || (chanspec & WL_CHAN_PASSIVE)) {
				DHD_ERROR(("Channel is radar sensitive\n"));
				return -1;
			}
			if (chanspec == 0) {
				DHD_ERROR(("Invalid hw channel\n"));
				return -1;
			}
		} else  {
			DHD_ERROR(("does not support per_chan_info\n"));
			return -1;
		}
		DHD_INFO(("non radar sensitivity\n"));
	}
	error = wldev_iovar_setbuf(dev, "csa", &csa_arg, sizeof(csa_arg),
		smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("%s:set csa failed:%d\n", __FUNCTION__, error));
		return -1;
	}
	return 0;
}
static int wl_android_get_band(struct net_device *dev, char *command, int total_len)
{
	uint band;
	int bytes_written;
	int error;

	error = wldev_get_band(dev, &band);
	if (error)
		return -1;
	bytes_written = snprintf(command, total_len, "Band %d", band);
	return bytes_written;
}

#ifdef CUSTOMER_HW4
#ifdef ROAM_API
int wl_android_set_roam_trigger(
	struct net_device *dev, char* command, int total_len)
{
	int roam_trigger[2];

	sscanf(command, "%*s %10d", &roam_trigger[0]);
	roam_trigger[1] = WLC_BAND_ALL;

	return wldev_ioctl(dev, WLC_SET_ROAM_TRIGGER, roam_trigger,
		sizeof(roam_trigger), 1);
}

static int wl_android_get_roam_trigger(
	struct net_device *dev, char *command, int total_len)
{
	int bytes_written;
	int roam_trigger[2] = {0, 0};

	roam_trigger[1] = WLC_BAND_2G;
	if (wldev_ioctl(dev, WLC_GET_ROAM_TRIGGER, roam_trigger,
		sizeof(roam_trigger), 0)) {
		roam_trigger[1] = WLC_BAND_5G;
		if (wldev_ioctl(dev, WLC_GET_ROAM_TRIGGER, roam_trigger,
			sizeof(roam_trigger), 0))
			return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_ROAMTRIGGER_GET, roam_trigger[0]);

	return bytes_written;
}

int wl_android_set_roam_delta(
	struct net_device *dev, char* command, int total_len)
{
	int roam_delta[2];

	sscanf(command, "%*s %10d", &roam_delta[0]);
	roam_delta[1] = WLC_BAND_ALL;

	return wldev_ioctl(dev, WLC_SET_ROAM_DELTA, roam_delta,
		sizeof(roam_delta), 1);
}

static int wl_android_get_roam_delta(
	struct net_device *dev, char *command, int total_len)
{
	int bytes_written;
	int roam_delta[2] = {0, 0};

	roam_delta[1] = WLC_BAND_2G;
	if (wldev_ioctl(dev, WLC_GET_ROAM_DELTA, roam_delta,
		sizeof(roam_delta), 0)) {
		roam_delta[1] = WLC_BAND_5G;
		if (wldev_ioctl(dev, WLC_GET_ROAM_DELTA, roam_delta,
			sizeof(roam_delta), 0))
			return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_ROAMDELTA_GET, roam_delta[0]);

	return bytes_written;
}

int wl_android_set_roam_scan_period(
	struct net_device *dev, char* command, int total_len)
{
	int roam_scan_period = 0;

	sscanf(command, "%*s %10d", &roam_scan_period);
	return wldev_ioctl(dev, WLC_SET_ROAM_SCAN_PERIOD, &roam_scan_period,
		sizeof(roam_scan_period), 1);
}

static int wl_android_get_roam_scan_period(
	struct net_device *dev, char *command, int total_len)
{
	int bytes_written;
	int roam_scan_period = 0;

	if (wldev_ioctl(dev, WLC_GET_ROAM_SCAN_PERIOD, &roam_scan_period,
		sizeof(roam_scan_period), 0))
		return -1;

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_ROAMSCANPERIOD_GET, roam_scan_period);

	return bytes_written;
}

int wl_android_set_full_roam_scan_period(
	struct net_device *dev, char* command, int total_len)
{
	int error = 0;
	int full_roam_scan_period = 0;
	char smbuf[WLC_IOCTL_SMLEN];

	sscanf(command+sizeof("SETFULLROAMSCANPERIOD"), "%d", &full_roam_scan_period);
	WL_TRACE(("fullroamperiod = %d\n", full_roam_scan_period));

	error = wldev_iovar_setbuf(dev, "fullroamperiod", &full_roam_scan_period,
		sizeof(full_roam_scan_period), smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("Failed to set full roam scan period, error = %d\n", error));
	}

	return error;
}

static int wl_android_get_full_roam_scan_period(
	struct net_device *dev, char *command, int total_len)
{
	int error;
	int bytes_written;
	int full_roam_scan_period = 0;

	error = wldev_iovar_getint(dev, "fullroamperiod", &full_roam_scan_period);

	if (error) {
		DHD_ERROR(("%s: get full roam scan period failed code %d\n",
			__func__, error));
		return -1;
	} else {
		DHD_INFO(("%s: get full roam scan period %d\n", __func__, full_roam_scan_period));
	}

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_FULLROAMSCANPERIOD_GET, full_roam_scan_period);

	return bytes_written;
}

int wl_android_set_country_rev(
	struct net_device *dev, char* command, int total_len)
{
	int error = 0;
	wl_country_t cspec = {{0}, 0, {0} };
	char country_code[WLC_CNTRY_BUF_SZ];
	char smbuf[WLC_IOCTL_SMLEN];
	int rev = 0;

	memset(country_code, 0, sizeof(country_code));
	sscanf(command+sizeof("SETCOUNTRYREV"), "%10s %10d", country_code, &rev);
	WL_TRACE(("country_code = %s, rev = %d\n", country_code, rev));

	memcpy(cspec.country_abbrev, country_code, sizeof(country_code));
	memcpy(cspec.ccode, country_code, sizeof(country_code));
	cspec.rev = rev;

	error = wldev_iovar_setbuf(dev, "country", (char *)&cspec,
		sizeof(cspec), smbuf, sizeof(smbuf), NULL);

	if (error) {
		DHD_ERROR(("%s: set country '%s/%d' failed code %d\n",
			__FUNCTION__, cspec.ccode, cspec.rev, error));
	} else {
		dhd_bus_country_set(dev, &cspec, true);
		DHD_INFO(("%s: set country '%s/%d'\n",
			__FUNCTION__, cspec.ccode, cspec.rev));
	}

	return error;
}

static int wl_android_get_country_rev(
	struct net_device *dev, char *command, int total_len)
{
	int error;
	int bytes_written;
	char smbuf[WLC_IOCTL_SMLEN];
	wl_country_t cspec;

	error = wldev_iovar_getbuf(dev, "country", NULL, 0, smbuf,
		sizeof(smbuf), NULL);

	if (error) {
		DHD_ERROR(("%s: get country failed code %d\n",
			__FUNCTION__, error));
		return -1;
	} else {
		memcpy(&cspec, smbuf, sizeof(cspec));
		DHD_INFO(("%s: get country '%c%c %d'\n",
			__FUNCTION__, cspec.ccode[0], cspec.ccode[1], cspec.rev));
	}

	bytes_written = snprintf(command, total_len, "%s %c%c %d",
		CMD_COUNTRYREV_GET, cspec.ccode[0], cspec.ccode[1], cspec.rev);

	return bytes_written;
}
#endif /* ROAM_API */

#ifdef WES_SUPPORT
int wl_android_get_roam_scan_control(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int mode = 0;

	error = get_roamscan_mode(dev, &mode);
	if (error) {
		DHD_ERROR(("%s: Failed to get Scan Control, error = %d\n", __FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETROAMSCANCONTROL, mode);

	return bytes_written;
}

int wl_android_set_roam_scan_control(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = set_roamscan_mode(dev, mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Control %d, error = %d\n",
		 __FUNCTION__, mode, error));
		return -1;
	}

	return 0;
}

int wl_android_get_roam_scan_channels(struct net_device *dev, char *command, int total_len)
{
	int bytes_written = 0;
	unsigned char channels[ANDROID_WIFI_MAX_ROAM_SCAN_CHANNELS] = {0};
	int channel_cnt = 0;
	char channel_info[10 + (ANDROID_WIFI_MAX_ROAM_SCAN_CHANNELS * 3)] = {0};
	int channel_info_len = 0;
	int i = 0;

	channel_cnt = get_roamscan_channel_list(dev, channels);

	channel_info_len += snprintf(&channel_info[channel_info_len], sizeof(channel_info),
	                             "%d ", channel_cnt);
	for (i = 0; i < channel_cnt; i++) {
		channel_info_len += snprintf(&channel_info[channel_info_len], sizeof(channel_info),
		                             "%d ", channels[i]);

		if (channel_info_len > (sizeof(channel_info) - 10))
			break;
	}
	channel_info_len += snprintf(&channel_info[channel_info_len], sizeof(channel_info),
	                             "%s", "\0");

	bytes_written = snprintf(command, total_len, "%s %s",
		CMD_GETROAMSCANCHANNELS, channel_info);
	return bytes_written;
}

int wl_android_set_roam_scan_channels(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	unsigned char *p = (unsigned char *)(command + strlen(CMD_SETROAMSCANCHANNELS) + 1);
	int ioctl_version = wl_cfg80211_get_ioctl_version();
	error = set_roamscan_channel_list(dev, p[0], &p[1], ioctl_version);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Channels %d, error = %d\n",
		 __FUNCTION__, p[0], error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_channel_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int time = 0;

	error = wldev_ioctl(dev, WLC_GET_SCAN_CHANNEL_TIME, &time, sizeof(time), 0);
	if (error) {
		DHD_ERROR(("%s: Failed to get Scan Channel Time, error = %d\n",
		__FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETSCANCHANNELTIME, time);

	return bytes_written;
}

int wl_android_set_scan_channel_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int time = 0;

	if (sscanf(command, "%*s %d", &time) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_ioctl(dev, WLC_SET_SCAN_CHANNEL_TIME, &time, sizeof(time), 1);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Channel Time %d, error = %d\n",
		__FUNCTION__, time, error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_home_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int time = 0;

	error = wldev_ioctl(dev, WLC_GET_SCAN_HOME_TIME, &time, sizeof(time), 0);
	if (error) {
		DHD_ERROR(("Failed to get Scan Home Time, error = %d\n", error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETSCANHOMETIME, time);

	return bytes_written;
}

int wl_android_set_scan_home_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int time = 0;

	if (sscanf(command, "%*s %d", &time) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_ioctl(dev, WLC_SET_SCAN_HOME_TIME, &time, sizeof(time), 1);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Home Time %d, error = %d\n",
		__FUNCTION__, time, error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_home_away_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int time = 0;

	error = wldev_iovar_getint(dev, "scan_home_away_time", &time);
	if (error) {
		DHD_ERROR(("%s: Failed to get Scan Home Away Time, error = %d\n",
		__FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETSCANHOMEAWAYTIME, time);

	return bytes_written;
}

int wl_android_set_scan_home_away_time(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int time = 0;

	if (sscanf(command, "%*s %d", &time) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_iovar_setint(dev, "scan_home_away_time", time);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Home Away Time %d, error = %d\n",
		 __FUNCTION__, time, error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_nprobes(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int num = 0;

	error = wldev_ioctl(dev, WLC_GET_SCAN_NPROBES, &num, sizeof(num), 0);
	if (error) {
		DHD_ERROR(("%s: Failed to get Scan NProbes, error = %d\n", __FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETSCANNPROBES, num);

	return bytes_written;
}

int wl_android_set_scan_nprobes(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int num = 0;

	if (sscanf(command, "%*s %d", &num) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_ioctl(dev, WLC_SET_SCAN_NPROBES, &num, sizeof(num), 1);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan NProbes %d, error = %d\n",
		__FUNCTION__, num, error));
		return -1;
	}

	return 0;
}

int wl_android_get_scan_dfs_channel_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int mode = 0;
	int scan_passive_time = 0;

	error = wldev_iovar_getint(dev, "scan_passive_time", &scan_passive_time);
	if (error) {
		DHD_ERROR(("%s: Failed to get Passive Time, error = %d\n", __FUNCTION__, error));
		return -1;
	}

	if (scan_passive_time == 0) {
		mode = 0;
	} else {
		mode = 1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETDFSSCANMODE, mode);

	return bytes_written;
}

int wl_android_set_scan_dfs_channel_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;
	int scan_passive_time = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	if (mode == 1) {
		scan_passive_time = DHD_SCAN_PASSIVE_TIME;
	} else if (mode == 0) {
		scan_passive_time = 0;
	} else {
		DHD_ERROR(("%s: Failed to set Scan DFS channel mode %d, error = %d\n",
		 __FUNCTION__, mode, error));
		return -1;
	}
	error = wldev_iovar_setint(dev, "scan_passive_time", scan_passive_time);
	if (error) {
		DHD_ERROR(("%s: Failed to set Scan Passive Time %d, error = %d\n",
		__FUNCTION__, scan_passive_time, error));
		return -1;
	}

	return 0;
}

#define JOINPREFFER_BUF_SIZE 12

static int
wl_android_set_join_prefer(struct net_device *dev, char *command, int total_len)
{
	int error = BCME_OK;
	char smbuf[WLC_IOCTL_SMLEN];
	uint8 buf[JOINPREFFER_BUF_SIZE];
	char *pcmd;
	int total_len_left;
	int i;
	char hex[2];

	pcmd = command + strlen(CMD_SETJOINPREFER) + 1;
	total_len_left = strlen(pcmd);

	memset(buf, 0, sizeof(buf));

	if (total_len_left != JOINPREFFER_BUF_SIZE << 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return BCME_ERROR;
	}

	/* Store the MSB first, as required by join_pref */
	for (i = 0; i < JOINPREFFER_BUF_SIZE; i++) {
		hex[0] = *pcmd++;
		hex[1] = *pcmd++;
		buf[i] = (uint8)simple_strtoul(hex, NULL, 16);
	}

	prhex("join pref", (uint8 *)buf, JOINPREFFER_BUF_SIZE);
	error = wldev_iovar_setbuf(dev, "join_pref", buf, JOINPREFFER_BUF_SIZE,
		smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("Failed to set join_pref, error = %d\n", error));
	}
	return error;
}

int wl_android_send_action_frame(struct net_device *dev, char *command, int total_len)
{
	int error = -1;
	android_wifi_af_params_t *params = NULL;
	wl_action_frame_t *action_frame = NULL;
	wl_af_params_t *af_params = NULL;
	char *smbuf = NULL;
	struct ether_addr tmp_bssid;
	int tmp_channel = 0;

	params = (android_wifi_af_params_t *)(command + strlen(CMD_SENDACTIONFRAME) + 1);

	smbuf = kmalloc(WLC_IOCTL_MAXLEN, GFP_KERNEL);
	if (smbuf == NULL) {
		DHD_ERROR(("%s: failed to allocated memory %d bytes\n",
		__FUNCTION__, WLC_IOCTL_MAXLEN));
		goto send_action_frame_out;
	}

	af_params = (wl_af_params_t *) kzalloc(WL_WIFI_AF_PARAMS_SIZE, GFP_KERNEL);
	if (af_params == NULL)
	{
		DHD_ERROR(("%s: unable to allocate frame\n", __FUNCTION__));
		goto send_action_frame_out;
	}

	memset(&tmp_bssid, 0, ETHER_ADDR_LEN);
	if (bcm_ether_atoe((const char *)params->bssid, (struct ether_addr *)&tmp_bssid) == 0) {
		memset(&tmp_bssid, 0, ETHER_ADDR_LEN);

		error = wldev_ioctl(dev, WLC_GET_BSSID, &tmp_bssid, ETHER_ADDR_LEN, false);
		if (error) {
			memset(&tmp_bssid, 0, ETHER_ADDR_LEN);
			DHD_ERROR(("%s: failed to get bssid, error=%d\n", __FUNCTION__, error));
			goto send_action_frame_out;
		}
	}

	if (params->channel < 0) {
		struct channel_info ci;
		error = wldev_ioctl(dev, WLC_GET_CHANNEL, &ci, sizeof(ci), false);
		if (error) {
			DHD_ERROR(("%s: failed to get channel, error=%d\n", __FUNCTION__, error));
			goto send_action_frame_out;
		}

		tmp_channel = ci.hw_channel;
	}
	else {
		tmp_channel = params->channel;
	}

	af_params->channel = tmp_channel;
	af_params->dwell_time = params->dwell_time;
	memcpy(&af_params->BSSID, &tmp_bssid, ETHER_ADDR_LEN);
	action_frame = &af_params->action_frame;

	action_frame->packetId = 0;
	memcpy(&action_frame->da, &tmp_bssid, ETHER_ADDR_LEN);
	action_frame->len = params->len;
	memcpy(action_frame->data, params->data, action_frame->len);

	error = wldev_iovar_setbuf(dev, "actframe", af_params,
		sizeof(wl_af_params_t), smbuf, WLC_IOCTL_MAXLEN, NULL);
	if (error) {
		DHD_ERROR(("%s: failed to set action frame, error=%d\n", __FUNCTION__, error));
	}

send_action_frame_out:
	if (af_params)
		kfree(af_params);

	if (smbuf)
		kfree(smbuf);

	if (error)
		return -1;
	else
		return 0;
}

int wl_android_reassoc(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	android_wifi_reassoc_params_t *params = NULL;
	uint band;
	chanspec_t channel;
	u32 params_size;
	wl_reassoc_params_t reassoc_params;

	params = (android_wifi_reassoc_params_t *)(command + strlen(CMD_REASSOC) + 1);

	memset(&reassoc_params, 0, WL_REASSOC_PARAMS_FIXED_SIZE);

	if (bcm_ether_atoe((const char *)params->bssid,
	(struct ether_addr *)&reassoc_params.bssid) == 0) {
		DHD_ERROR(("%s: Invalid bssid \n", __FUNCTION__));
		return -1;
	}

	if (params->channel < 0) {
		DHD_ERROR(("%s: Invalid Channel \n", __FUNCTION__));
		return -1;
	}

	reassoc_params.chanspec_num = 1;

	channel = params->channel;
#ifdef D11AC_IOTYPES
	if (wl_cfg80211_get_ioctl_version() == 1) {
		band = ((channel <= CH_MAX_2G_CHANNEL) ?
		WL_LCHANSPEC_BAND_2G : WL_LCHANSPEC_BAND_5G);
		reassoc_params.chanspec_list[0] = channel |
		band | WL_LCHANSPEC_BW_20 | WL_LCHANSPEC_CTL_SB_NONE;
	}
	else {
		band = ((channel <= CH_MAX_2G_CHANNEL) ? WL_CHANSPEC_BAND_2G : WL_CHANSPEC_BAND_5G);
		reassoc_params.chanspec_list[0] = channel | band | WL_CHANSPEC_BW_20;
	}
#else
	band = ((channel <= CH_MAX_2G_CHANNEL) ? WL_CHANSPEC_BAND_2G : WL_CHANSPEC_BAND_5G);
	reassoc_params.chanspec_list[0] = channel |
	band | WL_CHANSPEC_BW_20 | WL_CHANSPEC_CTL_SB_NONE;
#endif /* D11AC_IOTYPES */
	params_size = WL_REASSOC_PARAMS_FIXED_SIZE + sizeof(chanspec_t);

	error = wldev_ioctl(dev, WLC_REASSOC, &reassoc_params, params_size, true);
	if (error) {
		DHD_ERROR(("%s: failed to reassoc, error=%d\n", __FUNCTION__, error));
	}

	if (error)
		return -1;
	else
		return 0;
}

int wl_android_get_wes_mode(struct net_device *dev, char *command, int total_len)
{
	int bytes_written = 0;
	int mode = 0;

	mode = wl_cfg80211_get_wes_mode();

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETWESMODE, mode);

	return bytes_written;
}

int wl_android_set_wes_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wl_cfg80211_set_wes_mode(mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set WES Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
		return -1;
	}

	return 0;
}

int wl_android_get_okc_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int mode = 0;

	error = wldev_iovar_getint(dev, "okc_enable", &mode);
	if (error) {
		DHD_ERROR(("%s: Failed to get OKC Mode, error = %d\n", __FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GETOKCMODE, mode);

	return bytes_written;
}

int wl_android_set_okc_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_iovar_setint(dev, "okc_enable", mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set OKC Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
		return -1;
	}

	if (mode)
		 wldev_iovar_setint(dev, "ccx_enable", 0);

	return error;
}
#endif /* WES_SUPPORT */
#endif /* CUSTOMER_HW4 */
static int wl_android_wbtext(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int data;

	sscanf(command+sizeof("WBTEXT_ENABLE"), "%d", &data);

	if (data)
		data = WL_BSSTRANS_POLICY_PRODUCT;

	error = wldev_iovar_setint(dev, "wnm_bsstrans_resp", data);
	if (error) {
		DHD_ERROR(("%s: Failed to set wnm_bsstrans_resp error = %d\n",
			__FUNCTION__, error));
	}
	return error;
}

#ifdef PNO_SUPPORT
#define PNO_PARAM_SIZE 50
#define VALUE_SIZE 50
#define LIMIT_STR_FMT  ("%50s %50s")
static int
wls_parse_batching_cmd(struct net_device *dev, char *command, int total_len)
{
	int err = BCME_OK;
	uint i, tokens;
	char *pos, *pos2, *token, *token2, *delim;
	char param[PNO_PARAM_SIZE], value[VALUE_SIZE];
	struct dhd_pno_batch_params batch_params;
	DHD_PNO(("%s: command=%s, len=%d\n", __FUNCTION__, command, total_len));
	if (total_len < strlen(CMD_WLS_BATCHING)) {
		DHD_ERROR(("%s argument=%d less min size\n", __FUNCTION__, total_len));
		err = BCME_ERROR;
		goto exit;
	}
	pos = command + strlen(CMD_WLS_BATCHING) + 1;
	memset(&batch_params, 0, sizeof(struct dhd_pno_batch_params));

	if (!strncmp(pos, PNO_BATCHING_SET, strlen(PNO_BATCHING_SET))) {
		pos += strlen(PNO_BATCHING_SET) + 1;
		while ((token = strsep(&pos, PNO_PARAMS_DELIMETER)) != NULL) {
			memset(param, 0, sizeof(param));
			memset(value, 0, sizeof(value));
			if (token == NULL || !*token)
				break;
			if (*token == '\0')
				continue;
			delim = strchr(token, PNO_PARAM_VALUE_DELLIMETER);
			if (delim != NULL)
				*delim = ' ';

			tokens = sscanf(token, LIMIT_STR_FMT, param, value);
			if (!strncmp(param, PNO_PARAM_SCANFREQ, strlen(PNO_PARAM_SCANFREQ))) {
				batch_params.scan_fr = simple_strtol(value, NULL, 0);
				DHD_PNO(("scan_freq : %d\n", batch_params.scan_fr));
			} else if (!strncmp(param, PNO_PARAM_BESTN, strlen(PNO_PARAM_BESTN))) {
				batch_params.bestn = simple_strtol(value, NULL, 0);
				DHD_PNO(("bestn : %d\n", batch_params.bestn));
			} else if (!strncmp(param, PNO_PARAM_MSCAN, strlen(PNO_PARAM_MSCAN))) {
				batch_params.mscan = simple_strtol(value, NULL, 0);
				DHD_PNO(("mscan : %d\n", batch_params.mscan));
			} else if (!strncmp(param, PNO_PARAM_CHANNEL, strlen(PNO_PARAM_CHANNEL))) {
				i = 0;
				pos2 = value;
				tokens = sscanf(value, "<%s>", value);
				if (tokens != 1) {
					err = BCME_ERROR;
					DHD_ERROR(("%s : invalid format for channel"
					" <> params\n", __FUNCTION__));
					goto exit;
				}
					while ((token2 = strsep(&pos2,
					PNO_PARAM_CHANNEL_DELIMETER)) != NULL) {
					if (token2 == NULL || !*token2)
						break;
					if (*token2 == '\0')
						continue;
					if (*token2 == 'A' || *token2 == 'B') {
						batch_params.band = (*token2 == 'A')?
							WLC_BAND_5G : WLC_BAND_2G;
						DHD_PNO(("band : %s\n",
							(*token2 == 'A')? "A" : "B"));
					} else {
						batch_params.chan_list[i++] =
						simple_strtol(token2, NULL, 0);
						batch_params.nchan++;
						DHD_PNO(("channel :%d\n",
						batch_params.chan_list[i-1]));
					}
				 }
			} else if (!strncmp(param, PNO_PARAM_RTT, strlen(PNO_PARAM_RTT))) {
				batch_params.rtt = simple_strtol(value, NULL, 0);
				DHD_PNO(("rtt : %d\n", batch_params.rtt));
			} else {
				DHD_ERROR(("%s : unknown param: %s\n", __FUNCTION__, param));
				err = BCME_ERROR;
				goto exit;
			}
		}
		err = dhd_dev_pno_set_for_batch(dev, &batch_params);
		if (err < 0) {
			DHD_ERROR(("failed to configure batch scan\n"));
		} else {
			memset(command, 0, total_len);
			err = snprintf(command, total_len, "%d", err);
		}
	} else if (!strncmp(pos, PNO_BATCHING_GET, strlen(PNO_BATCHING_GET))) {
		err = dhd_dev_pno_get_for_batch(dev, command, total_len);
		if (err < 0) {
			DHD_ERROR(("failed to getting batching results\n"));
		} else {
			err = strlen(command);
		}
	} else if (!strncmp(pos, PNO_BATCHING_STOP, strlen(PNO_BATCHING_STOP))) {
		err = dhd_dev_pno_stop_for_batch(dev);
		if (err < 0) {
			DHD_ERROR(("failed to stop batching scan\n"));
		} else {
			memset(command, 0, total_len);
			err = snprintf(command, total_len, "OK");
		}
	} else {
		DHD_ERROR(("%s : unknown command\n", __FUNCTION__));
		err = BCME_ERROR;
		goto exit;
	}
exit:
	return err;
}
#if !defined(WL_SCHED_SCAN) && defined(OEM_ANDROID) && defined(WL_WIRELESS_EXT)
static int wl_android_set_pno_setup(struct net_device *dev, char *command, int total_len)
{
	wlc_ssid_ext_t ssids_local[MAX_PFN_LIST_COUNT];
	int res = -1;
	int nssid = 0;
	cmd_tlv_t *cmd_tlv_temp;
	char *str_ptr;
	int tlv_size_left;
	int pno_time = 0;
	int pno_repeat = 0;
	int pno_freq_expo_max = 0;

#ifdef PNO_SET_DEBUG
	int i;
	char pno_in_example[] = {
		'P', 'N', 'O', 'S', 'E', 'T', 'U', 'P', ' ',
		'S', '1', '2', '0',
		'S',
		0x05,
		'd', 'l', 'i', 'n', 'k',
		'S',
		0x04,
		'G', 'O', 'O', 'G',
		'T',
		'0', 'B',
		'R',
		'2',
		'M',
		'2',
		0x00
		};
#endif /* PNO_SET_DEBUG */
	DHD_PNO(("%s: command=%s, len=%d\n", __FUNCTION__, command, total_len));

	if (total_len < (strlen(CMD_PNOSETUP_SET) + sizeof(cmd_tlv_t))) {
		DHD_ERROR(("%s argument=%d less min size\n", __FUNCTION__, total_len));
		goto exit_proc;
	}
#ifdef PNO_SET_DEBUG
	memcpy(command, pno_in_example, sizeof(pno_in_example));
	total_len = sizeof(pno_in_example);
#endif
	str_ptr = command + strlen(CMD_PNOSETUP_SET);
	tlv_size_left = total_len - strlen(CMD_PNOSETUP_SET);

	cmd_tlv_temp = (cmd_tlv_t *)str_ptr;
	memset(ssids_local, 0, sizeof(ssids_local));

	if ((cmd_tlv_temp->prefix == PNO_TLV_PREFIX) &&
		(cmd_tlv_temp->version == PNO_TLV_VERSION) &&
		(cmd_tlv_temp->subtype == PNO_TLV_SUBTYPE_LEGACY_PNO)) {

		str_ptr += sizeof(cmd_tlv_t);
		tlv_size_left -= sizeof(cmd_tlv_t);

		if ((nssid = wl_iw_parse_ssid_list_tlv(&str_ptr, ssids_local,
			MAX_PFN_LIST_COUNT, &tlv_size_left)) <= 0) {
			DHD_ERROR(("SSID is not presented or corrupted ret=%d\n", nssid));
			goto exit_proc;
		} else {
			if ((str_ptr[0] != PNO_TLV_TYPE_TIME) || (tlv_size_left <= 1)) {
				DHD_ERROR(("%s scan duration corrupted field size %d\n",
					__FUNCTION__, tlv_size_left));
				goto exit_proc;
			}
			str_ptr++;
			pno_time = simple_strtoul(str_ptr, &str_ptr, 16);
			DHD_PNO(("%s: pno_time=%d\n", __FUNCTION__, pno_time));

			if (str_ptr[0] != 0) {
				if ((str_ptr[0] != PNO_TLV_FREQ_REPEAT)) {
					DHD_ERROR(("%s pno repeat : corrupted field\n",
						__FUNCTION__));
					goto exit_proc;
				}
				str_ptr++;
				pno_repeat = simple_strtoul(str_ptr, &str_ptr, 16);
				DHD_PNO(("%s :got pno_repeat=%d\n", __FUNCTION__, pno_repeat));
				if (str_ptr[0] != PNO_TLV_FREQ_EXPO_MAX) {
					DHD_ERROR(("%s FREQ_EXPO_MAX corrupted field size\n",
						__FUNCTION__));
					goto exit_proc;
				}
				str_ptr++;
				pno_freq_expo_max = simple_strtoul(str_ptr, &str_ptr, 16);
				DHD_PNO(("%s: pno_freq_expo_max=%d\n",
					__FUNCTION__, pno_freq_expo_max));
			}
		}
	} else {
		DHD_ERROR(("%s get wrong TLV command\n", __FUNCTION__));
		goto exit_proc;
	}

	res = dhd_dev_pno_set_for_ssid(dev, ssids_local, nssid, pno_time, pno_repeat,
		pno_freq_expo_max, NULL, 0);
exit_proc:
	return res;
}
#endif /* !WL_SCHED_SCAN && OEM_ANDROID && WL_WIRELESS_EXT */
#endif /* PNO_SUPPORT  */

static int wl_android_get_p2p_dev_addr(struct net_device *ndev, char *command, int total_len)
{
	int ret;
	int bytes_written = 0;

	ret = wl_cfg80211_get_p2p_dev_addr(ndev, (struct ether_addr*)command);
	if (ret)
		return 0;
	bytes_written = sizeof(struct ether_addr);
	return bytes_written;
}

#ifdef BCMCCX
static int wl_android_get_cckm_rn(struct net_device *dev, char *command)
{
	int error, rn;

	WL_TRACE(("%s:wl_android_get_cckm_rn\n", dev->name));

	error = wldev_iovar_getint(dev, "cckm_rn", &rn);
	if (unlikely(error)) {
		WL_ERR(("wl_android_get_cckm_rn error (%d)\n", error));
		return -1;
	}
	memcpy(command, &rn, sizeof(int));

	return sizeof(int);
}

static int wl_android_set_cckm_krk(struct net_device *dev, char *command)
{
	int error;
	unsigned char key[16];
	static char iovar_buf[WLC_IOCTL_MEDLEN];

	WL_TRACE(("%s: wl_iw_set_cckm_krk\n", dev->name));

	memset(iovar_buf, 0, sizeof(iovar_buf));
	memcpy(key, command+strlen("set cckm_krk")+1, 16);

	error = wldev_iovar_setbuf(dev, "cckm_krk", key, sizeof(key),
		iovar_buf, WLC_IOCTL_MEDLEN, NULL);
	if (unlikely(error))
	{
		WL_ERR((" cckm_krk set error (%d)\n", error));
		return -1;
	}
	return 0;
}

static int wl_android_get_assoc_res_ies(struct net_device *dev, char *command)
{
	int error;
	u8 buf[WL_ASSOC_INFO_MAX];
	wl_assoc_info_t assoc_info;
	u32 resp_ies_len = 0;
	int bytes_written = 0;

	WL_TRACE(("%s: wl_iw_get_assoc_res_ies\n", dev->name));

	error = wldev_iovar_getbuf(dev, "assoc_info", NULL, 0, buf, WL_ASSOC_INFO_MAX, NULL);
	if (unlikely(error)) {
		WL_ERR(("could not get assoc info (%d)\n", error));
		return -1;
	}

	memcpy(&assoc_info, buf, sizeof(wl_assoc_info_t));
	assoc_info.req_len = htod32(assoc_info.req_len);
	assoc_info.resp_len = htod32(assoc_info.resp_len);
	assoc_info.flags = htod32(assoc_info.flags);

	if (assoc_info.resp_len) {
		resp_ies_len = assoc_info.resp_len - sizeof(struct dot11_assoc_resp);
	}

	/* first 4 bytes are ie len */
	memcpy(command, &resp_ies_len, sizeof(u32));
	bytes_written = sizeof(u32);

	/* get the association resp IE's if there are any */
	if (resp_ies_len) {
		error = wldev_iovar_getbuf(dev, "assoc_resp_ies", NULL, 0,
			buf, WL_ASSOC_INFO_MAX, NULL);
		if (unlikely(error)) {
			WL_ERR(("could not get assoc resp_ies (%d)\n", error));
			return -1;
		}

		memcpy(command+sizeof(u32), buf, resp_ies_len);
		bytes_written += resp_ies_len;
	}
	return bytes_written;
}

#endif /* BCMCCX */

int
wl_android_set_ap_mac_list(struct net_device *dev, int macmode, struct maclist *maclist)
{
	int i, j, match;
	int ret	= 0;
	char mac_buf[MAX_NUM_OF_ASSOCLIST *
		sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;

	/* set filtering mode */
	if ((ret = wldev_ioctl(dev, WLC_SET_MACMODE, &macmode, sizeof(macmode), true)) != 0) {
		DHD_ERROR(("%s : WLC_SET_MACMODE error=%d\n", __FUNCTION__, ret));
		return ret;
	}
	if (macmode != MACLIST_MODE_DISABLED) {
		/* set the MAC filter list */
		if ((ret = wldev_ioctl(dev, WLC_SET_MACLIST, maclist,
			sizeof(int) + sizeof(struct ether_addr) * maclist->count, true)) != 0) {
			DHD_ERROR(("%s : WLC_SET_MACLIST error=%d\n", __FUNCTION__, ret));
			return ret;
		}
		/* get the current list of associated STAs */
		assoc_maclist->count = MAX_NUM_OF_ASSOCLIST;
		if ((ret = wldev_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist,
			sizeof(mac_buf), false)) != 0) {
			DHD_ERROR(("%s : WLC_GET_ASSOCLIST error=%d\n", __FUNCTION__, ret));
			return ret;
		}
		/* do we have any STA associated?  */
		if (assoc_maclist->count) {
			/* iterate each associated STA */
			for (i = 0; i < assoc_maclist->count; i++) {
				match = 0;
				/* compare with each entry */
				for (j = 0; j < maclist->count; j++) {
					DHD_INFO(("%s : associated="MACDBG " list="MACDBG "\n",
					__FUNCTION__, MAC2STRDBG(assoc_maclist->ea[i].octet),
					MAC2STRDBG(maclist->ea[j].octet)));
					if (memcmp(assoc_maclist->ea[i].octet,
						maclist->ea[j].octet, ETHER_ADDR_LEN) == 0) {
						match = 1;
						break;
					}
				}
				/* do conditional deauth */
				/*   "if not in the allow list" or "if in the deny list" */
				if ((macmode == MACLIST_MODE_ALLOW && !match) ||
					(macmode == MACLIST_MODE_DENY && match)) {
					scb_val_t scbval;

					scbval.val = htod32(1);
					memcpy(&scbval.ea, &assoc_maclist->ea[i],
						ETHER_ADDR_LEN);
					if ((ret = wldev_ioctl(dev,
						WLC_SCB_DEAUTHENTICATE_FOR_REASON,
						&scbval, sizeof(scb_val_t), true)) != 0)
						DHD_ERROR(("%s WLC_SCB_DEAUTHENTICATE error=%d\n",
							__FUNCTION__, ret));
				}
			}
		}
	}
	return ret;
}

/*
 * HAPD_MAC_FILTER mac_mode mac_cnt mac_addr1 mac_addr2
 *
 */
static int
wl_android_set_mac_address_filter(struct net_device *dev, const char* str)
{
	int i;
	int ret = 0;
	int macnum = 0;
	int macmode = MACLIST_MODE_DISABLED;
	struct maclist *list;
	char eabuf[ETHER_ADDR_STR_LEN];
	char *token;

	/* string should look like below (macmode/macnum/maclist) */
	/*   1 2 00:11:22:33:44:55 00:11:22:33:44:ff  */

	/* get the MAC filter mode */
	token = strsep((char**)&str, " ");
	if (!token) {
		return -1;
	}
	macmode = bcm_atoi(token);

	if (macmode < MACLIST_MODE_DISABLED || macmode > MACLIST_MODE_ALLOW) {
		DHD_ERROR(("%s : invalid macmode %d\n", __FUNCTION__, macmode));
		return -1;
	}

	token = strsep((char**)&str, " ");
	if (!token) {
		return -1;
	}
	macnum = bcm_atoi(token);
	if (macnum < 0 || macnum > MAX_NUM_MAC_FILT) {
		DHD_ERROR(("%s : invalid number of MAC address entries %d\n",
			__FUNCTION__, macnum));
		return -1;
	}
	/* allocate memory for the MAC list */
	list = (struct maclist*)kmalloc(sizeof(int) +
		sizeof(struct ether_addr) * macnum, GFP_KERNEL);
	if (!list) {
		DHD_ERROR(("%s : failed to allocate memory\n", __FUNCTION__));
		return -1;
	}
	/* prepare the MAC list */
	list->count = htod32(macnum);
	bzero((char *)eabuf, ETHER_ADDR_STR_LEN);
	for (i = 0; i < list->count; i++) {
		strncpy(eabuf, strsep((char**)&str, " "), ETHER_ADDR_STR_LEN - 1);
		if (!(ret = bcm_ether_atoe(eabuf, &list->ea[i]))) {
			DHD_ERROR(("%s : mac parsing err index=%d, addr=%s\n",
				__FUNCTION__, i, eabuf));
			list->count--;
			break;
		}
		DHD_INFO(("%s : %d/%d MACADDR=%s", __FUNCTION__, i, list->count, eabuf));
	}
	/* set the list */
	if ((ret = wl_android_set_ap_mac_list(dev, macmode, list)) != 0)
		DHD_ERROR(("%s : Setting MAC list failed error=%d\n", __FUNCTION__, ret));

	kfree(list);

	return 0;
}

/**
 * Global function definitions (declared in wl_android.h)
 */

int wl_android_wifi_on(struct net_device *dev)
{
	int ret = 0;
	int retry = POWERUP_MAX_RETRY;

	DHD_ERROR(("%s in\n", __FUNCTION__));
	if (!dev) {
		DHD_ERROR(("%s: dev is null\n", __FUNCTION__));
		return -EINVAL;
	}

	dhd_net_if_lock(dev);
	if (!g_wifi_on) {
		do {
			dhd_net_wifi_platform_set_power(dev, TRUE, WIFI_TURNON_DELAY);
			ret = dhd_net_bus_resume(dev, 0);
#ifdef BCMPCIE
			ret = dhd_net_bus_devreset(dev, FALSE);
#endif /* BCMPCIE */
			if (ret == 0) {
				break;
			}
			DHD_ERROR(("\nfailed to power up wifi chip, retry again (%d left) **\n\n",
				retry));
#ifdef BCMPCIE
			dhd_net_bus_devreset(dev, TRUE);
#endif /* BCMPCIE */
			dhd_net_wifi_platform_set_power(dev, FALSE, WIFI_TURNOFF_DELAY);
		} while (retry-- > 0);
		if (ret != 0) {
			DHD_ERROR(("\nfailed to power up wifi chip, max retry reached **\n\n"));
			goto exit;
		}
		ret = dhd_net_bus_devreset(dev, FALSE);
		dhd_net_bus_resume(dev, 1);

#ifndef BCMPCIE
		if (!ret) {
			if (dhd_dev_init_ioctl(dev) < 0) {
				ret = -EFAULT;
			}
		}
#endif /* !BCMPCIE */
		g_wifi_on = TRUE;
	}

exit:
	dhd_net_if_unlock(dev);

	return ret;
}

int wl_android_wifi_off(struct net_device *dev, bool on_failure)
{
	int ret = 0;

	DHD_ERROR(("%s in\n", __FUNCTION__));
	if (!dev) {
		DHD_TRACE(("%s: dev is null\n", __FUNCTION__));
		return -EINVAL;
	}

	dhd_net_if_lock(dev);
	if (g_wifi_on || on_failure) {
		ret = dhd_net_bus_devreset(dev, TRUE);
		dhd_net_bus_suspend(dev);
		dhd_net_wifi_platform_set_power(dev, FALSE, WIFI_TURNOFF_DELAY);
		g_wifi_on = FALSE;
	}
	dhd_net_if_unlock(dev);

	return ret;
}

static int wl_android_set_fwpath(struct net_device *net, char *command, int total_len)
{
	if ((strlen(command) - strlen(CMD_SETFWPATH)) > MOD_PARAM_PATHLEN)
		return -1;
	return dhd_net_set_fw_path(net, command + strlen(CMD_SETFWPATH) + 1);
}

#ifdef CONNECTION_STATISTICS
static int
wl_chanim_stats(struct net_device *dev, u8 *chan_idle)
{
	int err;
	wl_chanim_stats_t *list;
	/* Parameter _and_ returned buffer of chanim_stats. */
	wl_chanim_stats_t param;
	u8 result[WLC_IOCTL_SMLEN];
	chanim_stats_t *stats;

	memset(&param, 0, sizeof(param));

	param.buflen = htod32(sizeof(wl_chanim_stats_t));
	param.count = htod32(WL_CHANIM_COUNT_ONE);

	if ((err = wldev_iovar_getbuf(dev, "chanim_stats", (char*)&param, sizeof(wl_chanim_stats_t),
		(char*)result, sizeof(result), 0)) < 0) {
		WL_ERR(("Failed to get chanim results %d \n", err));
		return err;
	}

	list = (wl_chanim_stats_t*)result;

	list->buflen = dtoh32(list->buflen);
	list->version = dtoh32(list->version);
	list->count = dtoh32(list->count);

	if (list->buflen == 0) {
		list->version = 0;
		list->count = 0;
	} else if (list->version != WL_CHANIM_STATS_VERSION) {
		WL_ERR(("Sorry, firmware has wl_chanim_stats version %d "
			"but driver supports only version %d.\n",
				list->version, WL_CHANIM_STATS_VERSION));
		list->buflen = 0;
		list->count = 0;
	}

	stats = list->stats;
	stats->glitchcnt = dtoh32(stats->glitchcnt);
	stats->badplcp = dtoh32(stats->badplcp);
	stats->chanspec = dtoh16(stats->chanspec);
	stats->timestamp = dtoh32(stats->timestamp);
	stats->chan_idle = dtoh32(stats->chan_idle);

	WL_INFORM(("chanspec: 0x%4x glitch: %d badplcp: %d idle: %d timestamp: %d\n",
		stats->chanspec, stats->glitchcnt, stats->badplcp, stats->chan_idle,
		stats->timestamp));

	*chan_idle = stats->chan_idle;

	return (err);
}

static int
wl_android_get_connection_stats(struct net_device *dev, char *command, int total_len)
{
	static char iovar_buf[WLC_IOCTL_MAXLEN];
	wl_cnt_wlc_t* wlc_cnt = NULL;
#ifndef DISABLE_IF_COUNTERS
	wl_if_stats_t* if_stats = NULL;
#endif /* DISABLE_IF_COUNTERS */

	int link_speed = 0;
	struct connection_stats *output;
	unsigned int bufsize = 0;
	int bytes_written = -1;
	int ret = 0;

	WL_INFORM(("%s: enter Get Connection Stats\n", __FUNCTION__));

	if (total_len <= 0) {
		WL_ERR(("%s: invalid buffer size %d\n", __FUNCTION__, total_len));
		goto error;
	}

	bufsize = total_len;
	if (bufsize < sizeof(struct connection_stats)) {
		WL_ERR(("%s: not enough buffer size, provided=%u, requires=%zu\n",
			__FUNCTION__, bufsize,
			sizeof(struct connection_stats)));
		goto error;
	}

	output = (struct connection_stats *)command;

#ifndef DISABLE_IF_COUNTERS
	if ((if_stats = kmalloc(sizeof(*if_stats), GFP_KERNEL)) == NULL) {
		WL_ERR(("%s(%d): kmalloc failed\n", __FUNCTION__, __LINE__));
		goto error;
	}
	memset(if_stats, 0, sizeof(*if_stats));

	ret = wldev_iovar_getbuf(dev, "if_counters", NULL, 0,
		(char *)if_stats, sizeof(*if_stats), NULL);
	if (ret) {
		WL_ERR(("%s: if_counters not supported ret=%d\n",
			__FUNCTION__, ret));

		/* In case if_stats IOVAR is not supported, get information from counters. */
#endif /* DISABLE_IF_COUNTERS */
		ret = wldev_iovar_getbuf(dev, "counters", NULL, 0,
			iovar_buf, WLC_IOCTL_MAXLEN, NULL);
		if (unlikely(ret)) {
			WL_ERR(("counters error (%d) - size = %zu\n", ret, sizeof(wl_cnt_wlc_t)));
			goto error;
		}
		ret = wl_cntbuf_to_xtlv_format(NULL, iovar_buf, WL_CNTBUF_MAX_SIZE, 0);
		if (ret != BCME_OK) {
			WL_ERR(("%s wl_cntbuf_to_xtlv_format ERR %d\n",
			__FUNCTION__, ret));
			goto error;
		}

		if (!(wlc_cnt = GET_WLCCNT_FROM_CNTBUF(iovar_buf))) {
			WL_ERR(("%s wlc_cnt NULL!\n", __FUNCTION__));
			goto error;
		}

		output->txframe   = dtoh32(wlc_cnt->txframe);
		output->txbyte    = dtoh32(wlc_cnt->txbyte);
		output->txerror   = dtoh32(wlc_cnt->txerror);
		output->rxframe   = dtoh32(wlc_cnt->rxframe);
		output->rxbyte    = dtoh32(wlc_cnt->rxbyte);
		output->txfail    = dtoh32(wlc_cnt->txfail);
		output->txretry   = dtoh32(wlc_cnt->txretry);
		output->txretrie  = dtoh32(wlc_cnt->txretrie);
		output->txrts     = dtoh32(wlc_cnt->txrts);
		output->txnocts   = dtoh32(wlc_cnt->txnocts);
		output->txexptime = dtoh32(wlc_cnt->txexptime);
#ifndef DISABLE_IF_COUNTERS
	} else {
		/* Populate from if_stats. */
		if (dtoh16(if_stats->version) > WL_IF_STATS_T_VERSION) {
			WL_ERR(("%s: incorrect version of wl_if_stats_t, expected=%u got=%u\n",
				__FUNCTION__,  WL_IF_STATS_T_VERSION, if_stats->version));
			goto error;
		}

		output->txframe   = (uint32)dtoh64(if_stats->txframe);
		output->txbyte    = (uint32)dtoh64(if_stats->txbyte);
		output->txerror   = (uint32)dtoh64(if_stats->txerror);
		output->rxframe   = (uint32)dtoh64(if_stats->rxframe);
		output->rxbyte    = (uint32)dtoh64(if_stats->rxbyte);
		output->txfail    = (uint32)dtoh64(if_stats->txfail);
		output->txretry   = (uint32)dtoh64(if_stats->txretry);
		output->txretrie  = (uint32)dtoh64(if_stats->txretrie);
		if (dtoh16(if_stats->length) > OFFSETOF(wl_if_stats_t, txexptime)) {
			output->txexptime = (uint32)dtoh64(if_stats->txexptime);
			output->txrts     = (uint32)dtoh64(if_stats->txrts);
			output->txnocts   = (uint32)dtoh64(if_stats->txnocts);
		} else {
			output->txexptime = 0;
			output->txrts     = 0;
			output->txnocts   = 0;
		}
	}
#endif /* DISABLE_IF_COUNTERS */

	/* link_speed is in kbps */
	ret = wldev_get_link_speed(dev, &link_speed);
	if (ret || link_speed < 0) {
		WL_ERR(("%s: wldev_get_link_speed() failed, ret=%d, speed=%d\n",
			__FUNCTION__, ret, link_speed));
		goto error;
	}

	output->txrate    = link_speed;

	/* Channel idle ratio. */
	if (wl_chanim_stats(dev, &(output->chan_idle)) < 0) {
		output->chan_idle = 0;
	};

	bytes_written = sizeof(struct connection_stats);

error:
#ifndef DISABLE_IF_COUNTERS
	if (if_stats) {
		kfree(if_stats);
	}
#endif /* DISABLE_IF_COUNTERS */

	return bytes_written;
}
#endif /* CONNECTION_STATISTICS */

static int
wl_android_set_pmk(struct net_device *dev, char *command, int total_len)
{
	uchar pmk[33];
	int error = 0;
	char smbuf[WLC_IOCTL_SMLEN];
#ifdef OKC_DEBUG
	int i = 0;
#endif

	bzero(pmk, sizeof(pmk));
	memcpy((char *)pmk, command + strlen("SET_PMK "), 32);
	error = wldev_iovar_setbuf(dev, "okc_info_pmk", pmk, 32, smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("Failed to set PMK for OKC, error = %d\n", error));
	}
#ifdef OKC_DEBUG
	DHD_ERROR(("PMK is "));
	for (i = 0; i < 32; i++)
		DHD_ERROR(("%02X ", pmk[i]));

	DHD_ERROR(("\n"));
#endif
	return error;
}

static int
wl_android_okc_enable(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	char okc_enable = 0;

	okc_enable = command[strlen(CMD_OKC_ENABLE) + 1] - '0';
	error = wldev_iovar_setint(dev, "okc_enable", okc_enable);
	if (error) {
		DHD_ERROR(("Failed to %s OKC, error = %d\n",
			okc_enable ? "enable" : "disable", error));
		return error;
	}

	error = wldev_iovar_setint(dev, "ccx_enable", 0);
	if (error) {
		DHD_ERROR(("Failed to disable CCX %d\n", error));
	}

	return error;
}


#ifdef CUSTOMER_HW4
#ifdef SUPPORT_AMPDU_MPDU_CMD
/* CMD_AMPDU_MPDU */
static int
wl_android_set_ampdu_mpdu(struct net_device *dev, const char* string_num)
{
	int err = 0;
	int ampdu_mpdu;

	ampdu_mpdu = bcm_atoi(string_num);

	if (ampdu_mpdu > 32) {
		DHD_ERROR(("%s : ampdu_mpdu MAX value is 32.\n", __FUNCTION__));
		return -1;
	}

	DHD_ERROR(("%s : ampdu_mpdu = %d\n", __FUNCTION__, ampdu_mpdu));
	err = wldev_iovar_setint(dev, "ampdu_mpdu", ampdu_mpdu);
	if (err < 0) {
		DHD_ERROR(("%s : ampdu_mpdu set error. %d\n", __FUNCTION__, err));
		return -1;
	}

	return 0;
}
#endif /* SUPPORT_AMPDU_MPDU_CMD */
#endif /* CUSTOMER_HW4 */

/* SoftAP feature */
#define APCS_BAND_2G_LEGACY1	20
#define APCS_BAND_2G_LEGACY2	0
#define APCS_BAND_AUTO		"band=auto"
#define APCS_BAND_2G		"band=2g"
#define APCS_BAND_5G		"band=5g"
#define APCS_MAX_2G_CHANNELS	11
#if defined(WL_SUPPORT_AUTO_CHANNEL)
static int
wl_android_set_auto_channel(struct net_device *dev, const char* cmd_str,
	char* command, int total_len)
{
	int channel = 0;
	int chosen = 0;
	int retry = 0;
	int ret = 0;
	int spect = 0;
	u8 *reqbuf = NULL;
	uint32 band = WLC_BAND_2G;
	uint32 buf_size;

	if (cmd_str) {
		DHD_INFO(("Command: %s len:%d \n", cmd_str, (int)strlen(cmd_str)));
		if (strncmp(cmd_str, APCS_BAND_AUTO, strlen(APCS_BAND_AUTO)) == 0) {
			band = WLC_BAND_AUTO;
		} else if (strncmp(cmd_str, APCS_BAND_5G, strlen(APCS_BAND_5G)) == 0) {
			band = WLC_BAND_5G;
		} else if (strncmp(cmd_str, APCS_BAND_2G, strlen(APCS_BAND_2G)) == 0) {
			band = WLC_BAND_2G;
		} else {
			/*
			 * For backward compatibility: Some platforms used to issue argument 20 or 0
			 * to enforce the 2G channel selection
			 */
			channel = bcm_atoi(cmd_str);
			if ((channel == APCS_BAND_2G_LEGACY1) ||
				(channel == APCS_BAND_2G_LEGACY2))
				band = WLC_BAND_2G;
			else {
				DHD_ERROR(("ACS: Invalid argument\n"));
				return -EINVAL;
			}
		}
	} else {
		/* If no argument is provided, default to 2G */
		DHD_ERROR(("%s: No argument given default to 2.4G scan", __func__));
		band = WLC_BAND_2G;
	}
	DHD_INFO(("%s : HAPD_AUTO_CHANNEL = %d, band=%d \n", __FUNCTION__, channel, band));

	reqbuf = kzalloc(CHANSPEC_BUF_SIZE, GFP_KERNEL);
	if (reqbuf == NULL) {
		WL_ERR(("failed to allocate chanspec buffer\n"));
		return -ENOMEM;
	}

	if ((ret = wldev_ioctl(dev, WLC_GET_SPECT_MANAGMENT, &spect, sizeof(spect), false)) < 0) {
		WL_ERR(("ACS:error getting the spect\n"));
		goto done;
	}

	if (spect > 0) {
		/* If STA is connected, return is STA channel, else ACS can be issued,
		 * set spect to 0 and proceed with ACS
		 */
		channel = 0;
		if ((channel = wl_cfg80211_get_sta_channel(dev)) > 0) {
			goto done2;
		} else if ((ret = wl_cfg80211_set_spect(dev, 0) < 0)) {
			WL_ERR(("ACS: error while setting spect\n"));
			goto done;
		}
	}

	if (band == WLC_BAND_AUTO) {
		DHD_INFO(("%s: ACS full channel scan \n", __func__));
		reqbuf[0] = htod32(0);
	} else if (band == WLC_BAND_5G) {
		DHD_INFO(("%s: ACS 5G band scan \n", __func__));
		if ((ret = wl_cfg80211_get_chanspecs_5g(dev, reqbuf, CHANSPEC_BUF_SIZE)) < 0) {
			WL_ERR(("ACS 5g chanspec retreival failed! \n"));
			goto done;
		}
	} else if (band == WLC_BAND_2G) {
		/*
		 * If channel argument is not provided/ argument 20 is provided,
		 * Restrict channel to 2GHz, 20MHz BW, No SB
		 */
		DHD_INFO(("%s: ACS 2G band scan \n", __func__));
		if ((ret = wl_cfg80211_get_chanspecs_2g(dev, reqbuf, CHANSPEC_BUF_SIZE)) < 0) {
			WL_ERR(("ACS 2g chanspec retreival failed! \n"));
			goto done;
		}
	} else {
		DHD_ERROR(("ACS: No band chosen\n"));
		goto done2;
	}

	buf_size = (band == WLC_BAND_AUTO) ? sizeof(int) : CHANSPEC_BUF_SIZE;
	ret = wldev_ioctl(dev, WLC_START_CHANNEL_SEL, (void *)reqbuf,
		buf_size, true);
	if (ret < 0) {
		DHD_ERROR(("%s: can't start auto channel scan, err = %d\n",
			__FUNCTION__, ret));
		channel = 0;
		goto done;
	}

	/* Wait for auto channel selection, max 3000 ms */
	if ((band == WLC_BAND_2G) || (band == WLC_BAND_5G)) {
		OSL_SLEEP(500);
	} else {
		/*
		 * Full channel scan at the minimum takes 1.2secs
		 * even with parallel scan. max wait time: 3500ms
		 */
		OSL_SLEEP(1000);
	}

	retry = 10;
	while (retry--) {
		ret = wldev_ioctl(dev, WLC_GET_CHANNEL_SEL, &chosen,
			sizeof(chosen), false);
		if (ret < 0 || dtoh32(chosen) == 0) {
			DHD_INFO(("%s: %d tried, ret = %d, chosen = %d\n",
				__FUNCTION__, (10 - retry), ret, chosen));
			OSL_SLEEP(250);
		} else {
#ifdef D11AC_IOTYPES
			if (wl_cfg80211_get_ioctl_version() == 1)
				channel = LCHSPEC_CHANNEL((u16)chosen);
			else
				channel = CHSPEC_CHANNEL((u16)chosen);
#else
			channel = CHSPEC_CHANNEL((u16)chosen);
#endif /* D11AC_IOTYPES */
			DHD_ERROR(("%s: selected channel = %d\n", __FUNCTION__, channel));
			break;
		}
	}

done:
	if ((retry == 0) || (ret < 0)) {
		/* On failure, fallback to a default channel */
		if ((band == WLC_BAND_5G)) {
			channel = 36;
		} else {
			channel = 1;
		}
		DHD_ERROR(("%s: ACS failed."
			" Fall back to default channel (%d) \n", __FUNCTION__, channel));
	}
done2:
	if (spect) {
		if ((ret = wl_cfg80211_set_spect(dev, spect) < 0)) {
			WL_ERR(("ACS: error while setting spect\n"));
		}
	}
	if (reqbuf)
		kfree(reqbuf);

	if (channel) {
		snprintf(command, 4, "%d", channel);
		DHD_INFO(("%s: command result is %s \n", __FUNCTION__, command));
		return strlen(command);
	} else {
		return ret;
	}
}
#endif /* WL_SUPPORT_AUTO_CHANNEL */

#ifdef CUSTOMER_HW4
#ifdef SUPPORT_HIDDEN_AP
static int
wl_android_set_max_num_sta(struct net_device *dev, const char* string_num)
{
	int max_assoc;

	max_assoc = bcm_atoi(string_num);
	DHD_INFO(("%s : HAPD_MAX_NUM_STA = %d\n", __FUNCTION__, max_assoc));
	wldev_iovar_setint(dev, "maxassoc", max_assoc);
	return 1;
}

static int
wl_android_set_ssid(struct net_device *dev, const char* hapd_ssid)
{
	wlc_ssid_t ssid;
	s32 ret;

	ssid.SSID_len = strlen(hapd_ssid);
	if (ssid.SSID_len > DOT11_MAX_SSID_LEN) {
		ssid.SSID_len = DOT11_MAX_SSID_LEN;
		DHD_ERROR(("%s : Too long SSID Length %zu\n", __FUNCTION__, strlen(hapd_ssid)));
	}
	bcm_strncpy_s(ssid.SSID, sizeof(ssid.SSID), hapd_ssid, ssid.SSID_len);
	DHD_INFO(("%s: HAPD_SSID = %s\n", __FUNCTION__, ssid.SSID));
	ret = wldev_ioctl(dev, WLC_SET_SSID, &ssid, sizeof(wlc_ssid_t), true);
	if (ret < 0) {
		DHD_ERROR(("%s : WLC_SET_SSID Error:%d\n", __FUNCTION__, ret));
	}
	return 1;

}

static int
wl_android_set_hide_ssid(struct net_device *dev, const char* string_num)
{
	int hide_ssid;
	int enable = 0;

	hide_ssid = bcm_atoi(string_num);
	DHD_INFO(("%s: HAPD_HIDE_SSID = %d\n", __FUNCTION__, hide_ssid));
	if (hide_ssid)
		enable = 1;
	wldev_iovar_setint(dev, "closednet", enable);
	return 1;
}
#endif /* SUPPORT_HIDDEN_AP */

#ifdef SUPPORT_SOFTAP_SINGL_DISASSOC
static int
wl_android_sta_diassoc(struct net_device *dev, const char* straddr)
{
	scb_val_t scbval;
	int error  = 0;

	DHD_INFO(("%s: deauth STA %s\n", __FUNCTION__, straddr));

	/* Unspecified reason */
	scbval.val = htod32(1);
	bcm_ether_atoe(straddr, &scbval.ea);

	if (bcm_ether_atoe(straddr, &scbval.ea) == 0) {
		DHD_ERROR(("%s: Invalid station MAC Address!!!\n", __FUNCTION__));
		return -1;
	}

	DHD_ERROR(("%s: deauth STA: "MACDBG " scb_val.val %d\n", __FUNCTION__,
		MAC2STRDBG(scbval.ea.octet), scbval.val));

	error = wldev_ioctl(dev, WLC_SCB_DEAUTHENTICATE_FOR_REASON, &scbval,
		sizeof(scb_val_t), true);
	if (error) {
		DHD_ERROR(("Fail to DEAUTH station, error = %d\n", error));
	}

	return 1;
}
#endif /* SUPPORT_SOFTAP_SINGL_DISASSOC */

#ifdef SUPPORT_SET_LPC
static int
wl_android_set_lpc(struct net_device *dev, const char* string_num)
{
	int lpc_enabled, ret;
	s32 val = 1;

	lpc_enabled = bcm_atoi(string_num);
	DHD_INFO(("%s : HAPD_LPC_ENABLED = %d\n", __FUNCTION__, lpc_enabled));

	ret = wldev_ioctl(dev, WLC_DOWN, &val, sizeof(s32), true);
	if (ret < 0)
		DHD_ERROR(("WLC_DOWN error %d\n", ret));

	wldev_iovar_setint(dev, "lpc", lpc_enabled);

	ret = wldev_ioctl(dev, WLC_UP, &val, sizeof(s32), true);
	if (ret < 0)
		DHD_ERROR(("WLC_UP error %d\n", ret));

	return 1;
}
#endif /* SUPPORT_SET_LPC */

static int
wl_android_ch_res_rl(struct net_device *dev, bool change)
{
	int error = 0;
	s32 srl = 7;
	s32 lrl = 4;
	printk("%s enter\n", __FUNCTION__);
	if (change) {
		srl = 4;
		lrl = 2;
	}
	error = wldev_ioctl(dev, WLC_SET_SRL, &srl, sizeof(s32), true);
	if (error) {
		DHD_ERROR(("Failed to set SRL, error = %d\n", error));
	}
	error = wldev_ioctl(dev, WLC_SET_LRL, &lrl, sizeof(s32), true);
	if (error) {
		DHD_ERROR(("Failed to set LRL, error = %d\n", error));
	}
	return error;
}

#ifdef SUPPORT_LTECX
#define DEFAULT_WLANRX_PROT	1
#define DEFAULT_LTERX_PROT	0
#define DEFAULT_LTETX_ADV	1200

static int
wl_android_set_ltecx(struct net_device *dev, const char* string_num)
{
	uint16 chan_bitmap;
	int ret;

	chan_bitmap = bcm_strtoul(string_num, NULL, 16);

	DHD_INFO(("%s : LTECOEX 0x%x\n", __FUNCTION__, chan_bitmap));

	if (chan_bitmap) {
		ret = wldev_iovar_setint(dev, "mws_coex_bitmap", chan_bitmap);
		if (ret < 0)
			DHD_ERROR(("mws_coex_bitmap error %d\n", ret));

		ret = wldev_iovar_setint(dev, "mws_wlanrx_prot", DEFAULT_WLANRX_PROT);
		if (ret < 0)
			DHD_ERROR(("mws_wlanrx_prot error %d\n", ret));

		ret = wldev_iovar_setint(dev, "mws_lterx_prot", DEFAULT_LTERX_PROT);
		if (ret < 0)
			DHD_ERROR(("mws_lterx_prot error %d\n", ret));

		ret = wldev_iovar_setint(dev, "mws_ltetx_adv", DEFAULT_LTETX_ADV);
		if (ret < 0)
			DHD_ERROR(("mws_ltetx_adv error %d\n", ret));
	} else {
		ret = wldev_iovar_setint(dev, "mws_coex_bitmap", chan_bitmap);
		if (ret < 0)
			DHD_ERROR(("LTECX_CHAN_BITMAP error %d\n", ret));
	}
	return 1;
}
#endif /* SUPPORT_LTECX */

static int
wl_android_rmc_enable(struct net_device *net, int rmc_enable)
{
	int err;

	err = wldev_iovar_setint(net, "rmc_ackreq", rmc_enable);
	return err;
}

static int
wl_android_rmc_set_leader(struct net_device *dev, const char* straddr)
{
	int error  = BCME_OK;
	char smbuf[WLC_IOCTL_SMLEN];
	wl_rmc_entry_t rmc_entry;
	DHD_INFO(("%s: Set new RMC leader %s\n", __FUNCTION__, straddr));

	memset(&rmc_entry, 0, sizeof(wl_rmc_entry_t));
	if (!bcm_ether_atoe(straddr, &rmc_entry.addr)) {
		if (strlen(straddr) == 1 && bcm_atoi(straddr) == 0) {
			DHD_INFO(("%s: Set auto leader selection mode\n", __FUNCTION__));
			memset(&rmc_entry, 0, sizeof(wl_rmc_entry_t));
		} else {
			DHD_ERROR(("%s: No valid mac address provided\n",
				__FUNCTION__));
			return BCME_ERROR;
		}
	}

	error = wldev_iovar_setbuf(dev, "rmc_ar", &rmc_entry, sizeof(wl_rmc_entry_t),
		smbuf, sizeof(smbuf), NULL);

	if (error != BCME_OK) {
		DHD_ERROR(("%s: Unable to set RMC leader, error = %d\n",
			__FUNCTION__, error));
	}

	return error;
}

static int wl_android_set_rmc_event(struct net_device *dev, char *command, int total_len)
{
	int err = 0;
	int pid = 0;

	if (sscanf(command, CMD_SET_RMC_EVENT " %d", &pid) <= 0) {
		WL_ERR(("Failed to get Parameter from : %s\n", command));
		return -1;
	}

	/* set pid, and if the event was happened, let's send a notification through netlink */
	wl_cfg80211_set_rmc_pid(pid);

	WL_DBG(("RMC pid=%d\n", pid));

	return err;
}

int wl_android_get_singlecore_scan(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int mode = 0;

	error = wldev_iovar_getint(dev, "scan_ps", &mode);
	if (error) {
		DHD_ERROR(("%s: Failed to get single core scan Mode, error = %d\n",
			__FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d", CMD_GET_SCSCAN, mode);

	return bytes_written;
}

int wl_android_set_singlecore_scan(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_iovar_setint(dev, "scan_ps", mode);
	if (error) {
		DHD_ERROR(("%s[1]: Failed to set Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
		return -1;
	}

	return error;
}
#ifdef TEST_TX_POWER_CONTROL
static int
wl_android_set_tx_power(struct net_device *dev, const char* string_num)
{
	int err = 0;
	s32 dbm;
	enum nl80211_tx_power_setting type;

	dbm = bcm_atoi(string_num);

	if (dbm < -1) {
		DHD_ERROR(("%s: dbm is negative...\n", __FUNCTION__));
		return -EINVAL;
	}

	if (dbm == -1)
		type = NL80211_TX_POWER_AUTOMATIC;
	else
		type = NL80211_TX_POWER_FIXED;

	err = wl_set_tx_power(dev, type, dbm);
	if (unlikely(err)) {
		DHD_ERROR(("%s: error (%d)\n", __FUNCTION__, err));
		return err;
	}

	return 1;
}

static int
wl_android_get_tx_power(struct net_device *dev, char *command, int total_len)
{
	int err;
	int bytes_written;
	s32 dbm = 0;

	err = wl_get_tx_power(dev, &dbm);
	if (unlikely(err)) {
		DHD_ERROR(("%s: error (%d)\n", __FUNCTION__, err));
		return err;
	}

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_TEST_GET_TX_POWER, dbm);

	DHD_ERROR(("%s: GET_TX_POWER: dBm=%d\n", __FUNCTION__, dbm));

	return bytes_written;
}
#endif /* TEST_TX_POWER_CONTROL */

static int
wl_android_set_sarlimit_txctrl(struct net_device *dev, const char* string_num)
{
	int err = 0;
	int setval = 0;
	s32 mode = bcm_atoi(string_num);

	/* As Samsung specific and their requirement, '0' means activate sarlimit
	 * and '-1' means back to normal state (deactivate sarlimit)
	 */
	if (mode == 0) {
		DHD_INFO(("%s: SAR limit control activated\n", __FUNCTION__));
		setval = 1;
	} else if (mode == -1) {
		DHD_INFO(("%s: SAR limit control deactivated\n", __FUNCTION__));
		setval = 0;
	} else {
		return -EINVAL;
	}

	err = wldev_iovar_setint(dev, "sar_enable", setval);
	if (unlikely(err)) {
		DHD_ERROR(("%s: error (%d)\n", __FUNCTION__, err));
		return err;
	}
	return 1;
}
#endif /* CUSTOMER_HW4 */

int wl_android_set_roam_mode(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int mode = 0;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	error = wldev_iovar_setint(dev, "roam_off", mode);
	if (error) {
		DHD_ERROR(("%s: Failed to set roaming Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
		return -1;
	}
	else
		DHD_ERROR(("%s: succeeded to set roaming Mode %d, error = %d\n",
		__FUNCTION__, mode, error));
	return 0;
}

int wl_android_set_ibss_beacon_ouidata(struct net_device *dev, char *command, int total_len)
{
	char ie_buf[VNDR_IE_MAX_LEN];
	char *ioctl_buf = NULL;
	char hex[] = "XX";
	char *pcmd = NULL;
	int ielen = 0, datalen = 0, idx = 0, tot_len = 0;
	vndr_ie_setbuf_t *vndr_ie = NULL;
	s32 iecount;
	uint32 pktflag;
	u16 kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	s32 err = BCME_OK;

	/* Check the VSIE (Vendor Specific IE) which was added.
	 *  If exist then send IOVAR to delete it
	 */
	if (wl_cfg80211_ibss_vsie_delete(dev) != BCME_OK) {
		return -EINVAL;
	}

	pcmd = command + strlen(CMD_SETIBSSBEACONOUIDATA) + 1;
	for (idx = 0; idx < DOT11_OUI_LEN; idx++) {
		hex[0] = *pcmd++;
		hex[1] = *pcmd++;
		ie_buf[idx] =  (uint8)simple_strtoul(hex, NULL, 16);
	}
	pcmd++;
	while ((*pcmd != '\0') && (idx < VNDR_IE_MAX_LEN)) {
		hex[0] = *pcmd++;
		hex[1] = *pcmd++;
		ie_buf[idx++] =  (uint8)simple_strtoul(hex, NULL, 16);
		datalen++;
	}
	tot_len = sizeof(vndr_ie_setbuf_t) + (datalen - 1);
	vndr_ie = (vndr_ie_setbuf_t *) kzalloc(tot_len, kflags);
	if (!vndr_ie) {
		WL_ERR(("IE memory alloc failed\n"));
		return -ENOMEM;
	}
	/* Copy the vndr_ie SET command ("add"/"del") to the buffer */
	strncpy(vndr_ie->cmd, "add", VNDR_IE_CMD_LEN - 1);
	vndr_ie->cmd[VNDR_IE_CMD_LEN - 1] = '\0';

	/* Set the IE count - the buffer contains only 1 IE */
	iecount = htod32(1);
	memcpy((void *)&vndr_ie->vndr_ie_buffer.iecount, &iecount, sizeof(s32));

	/* Set packet flag to indicate that BEACON's will contain this IE */
	pktflag = htod32(VNDR_IE_BEACON_FLAG | VNDR_IE_PRBRSP_FLAG);
	memcpy((void *)&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].pktflag, &pktflag,
		sizeof(u32));
	/* Set the IE ID */
	vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.id = (uchar) DOT11_MNG_PROPR_ID;

	memcpy(&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.oui, &ie_buf,
		DOT11_OUI_LEN);
	memcpy(&vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.data,
		&ie_buf[DOT11_OUI_LEN], datalen);

	ielen = DOT11_OUI_LEN + datalen;
	vndr_ie->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.len = (uchar) ielen;

	ioctl_buf = kmalloc(WLC_IOCTL_MEDLEN, GFP_KERNEL);
	if (!ioctl_buf) {
		WL_ERR(("ioctl memory alloc failed\n"));
		if (vndr_ie) {
			kfree(vndr_ie);
		}
		return -ENOMEM;
	}
	memset(ioctl_buf, 0, WLC_IOCTL_MEDLEN);	/* init the buffer */
	err = wldev_iovar_setbuf(dev, "ie", vndr_ie, tot_len, ioctl_buf, WLC_IOCTL_MEDLEN, NULL);


	if (err != BCME_OK) {
		err = -EINVAL;
		if (vndr_ie) {
			kfree(vndr_ie);
		}
	}
	else {
		/* do NOT free 'vndr_ie' for the next process */
		wl_cfg80211_ibss_vsie_set_buffer(vndr_ie, tot_len);
	}

	if (ioctl_buf) {
		kfree(ioctl_buf);
	}

	return err;
}

#if defined(BCMFW_ROAM_ENABLE)
static int
wl_android_set_roampref(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	char smbuf[WLC_IOCTL_SMLEN];
	uint8 buf[MAX_BUF_SIZE];
	uint8 *pref = buf;
	char *pcmd;
	int num_ucipher_suites = 0;
	int num_akm_suites = 0;
	wpa_suite_t ucipher_suites[MAX_NUM_SUITES];
	wpa_suite_t akm_suites[MAX_NUM_SUITES];
	int num_tuples = 0;
	int total_bytes = 0;
	int total_len_left;
	int i, j;
	char hex[] = "XX";

	pcmd = command + strlen(CMD_SET_ROAMPREF) + 1;
	total_len_left = total_len - strlen(CMD_SET_ROAMPREF) + 1;

	num_akm_suites = simple_strtoul(pcmd, NULL, 16);
	/* Increment for number of AKM suites field + space */
	pcmd += 3;
	total_len_left -= 3;

	/* check to make sure pcmd does not overrun */
	if (total_len_left < (num_akm_suites * WIDTH_AKM_SUITE))
		return -1;

	memset(buf, 0, sizeof(buf));
	memset(akm_suites, 0, sizeof(akm_suites));
	memset(ucipher_suites, 0, sizeof(ucipher_suites));

	/* Save the AKM suites passed in the command */
	for (i = 0; i < num_akm_suites; i++) {
		/* Store the MSB first, as required by join_pref */
		for (j = 0; j < 4; j++) {
			hex[0] = *pcmd++;
			hex[1] = *pcmd++;
			buf[j] = (uint8)simple_strtoul(hex, NULL, 16);
		}
		memcpy((uint8 *)&akm_suites[i], buf, sizeof(uint32));
	}

	total_len_left -= (num_akm_suites * WIDTH_AKM_SUITE);
	num_ucipher_suites = simple_strtoul(pcmd, NULL, 16);
	/* Increment for number of cipher suites field + space */
	pcmd += 3;
	total_len_left -= 3;

	if (total_len_left < (num_ucipher_suites * WIDTH_AKM_SUITE))
		return -1;

	/* Save the cipher suites passed in the command */
	for (i = 0; i < num_ucipher_suites; i++) {
		/* Store the MSB first, as required by join_pref */
		for (j = 0; j < 4; j++) {
			hex[0] = *pcmd++;
			hex[1] = *pcmd++;
			buf[j] = (uint8)simple_strtoul(hex, NULL, 16);
		}
		memcpy((uint8 *)&ucipher_suites[i], buf, sizeof(uint32));
	}

	/* Join preference for RSSI
	 * Type	  : 1 byte (0x01)
	 * Length : 1 byte (0x02)
	 * Value  : 2 bytes	(reserved)
	 */
	*pref++ = WL_JOIN_PREF_RSSI;
	*pref++ = JOIN_PREF_RSSI_LEN;
	*pref++ = 0;
	*pref++ = 0;

	/* Join preference for WPA
	 * Type	  : 1 byte (0x02)
	 * Length : 1 byte (not used)
	 * Value  : (variable length)
	 *		reserved: 1 byte
	 *      count	: 1 byte (no of tuples)
	 *		Tuple1	: 12 bytes
	 *			akm[4]
	 *			ucipher[4]
	 *			mcipher[4]
	 *		Tuple2	: 12 bytes
	 *		Tuplen	: 12 bytes
	 */
	num_tuples = num_akm_suites * num_ucipher_suites;
	if (num_tuples != 0) {
		if (num_tuples <= JOIN_PREF_MAX_WPA_TUPLES) {
			*pref++ = WL_JOIN_PREF_WPA;
			*pref++ = 0;
			*pref++ = 0;
			*pref++ = (uint8)num_tuples;
			total_bytes = JOIN_PREF_RSSI_SIZE + JOIN_PREF_WPA_HDR_SIZE +
				(JOIN_PREF_WPA_TUPLE_SIZE * num_tuples);
		} else {
			DHD_ERROR(("%s: Too many wpa configs for join_pref \n", __FUNCTION__));
			return -1;
		}
	} else {
		/* No WPA config, configure only RSSI preference */
		total_bytes = JOIN_PREF_RSSI_SIZE;
	}

	/* akm-ucipher-mcipher tuples in the format required for join_pref */
	for (i = 0; i < num_ucipher_suites; i++) {
		for (j = 0; j < num_akm_suites; j++) {
			memcpy(pref, (uint8 *)&akm_suites[j], WPA_SUITE_LEN);
			pref += WPA_SUITE_LEN;
			memcpy(pref, (uint8 *)&ucipher_suites[i], WPA_SUITE_LEN);
			pref += WPA_SUITE_LEN;
			/* Set to 0 to match any available multicast cipher */
			memset(pref, 0, WPA_SUITE_LEN);
			pref += WPA_SUITE_LEN;
		}
	}

	prhex("join pref", (uint8 *)buf, total_bytes);
	error = wldev_iovar_setbuf(dev, "join_pref", buf, total_bytes, smbuf, sizeof(smbuf), NULL);
	if (error) {
		DHD_ERROR(("Failed to set join_pref, error = %d\n", error));
	}
	return error;
}
#endif /* defined(BCMFW_ROAM_ENABLE */

static int
wl_android_iolist_add(struct net_device *dev, struct list_head *head, struct io_cfg *config)
{
	struct io_cfg *resume_cfg;
	s32 ret;

	resume_cfg = kzalloc(sizeof(struct io_cfg), GFP_KERNEL);
	if (!resume_cfg)
		return -ENOMEM;

	if (config->iovar) {
		ret = wldev_iovar_getint(dev, config->iovar, &resume_cfg->param);
		if (ret) {
			DHD_ERROR(("%s: Failed to get current %s value\n",
				__FUNCTION__, config->iovar));
			goto error;
		}

		ret = wldev_iovar_setint(dev, config->iovar, config->param);
		if (ret) {
			DHD_ERROR(("%s: Failed to set %s to %d\n", __FUNCTION__,
				config->iovar, config->param));
			goto error;
		}

		resume_cfg->iovar = config->iovar;
	} else {
		resume_cfg->arg = kzalloc(config->len, GFP_KERNEL);
		if (!resume_cfg->arg) {
			ret = -ENOMEM;
			goto error;
		}
		ret = wldev_ioctl(dev, config->ioctl, resume_cfg->arg, config->len, false);
		if (ret) {
			DHD_ERROR(("%s: Failed to get ioctl %d\n", __FUNCTION__,
				config->ioctl));
			goto error;
		}
		ret = wldev_ioctl(dev, config->ioctl + 1, config->arg, config->len, true);
		if (ret) {
			DHD_ERROR(("%s: Failed to set %s to %d\n", __FUNCTION__,
				config->iovar, config->param));
			goto error;
		}
		if (config->ioctl + 1 == WLC_SET_PM)
			wl_cfg80211_update_power_mode(dev);
		resume_cfg->ioctl = config->ioctl;
		resume_cfg->len = config->len;
	}

	list_add(&resume_cfg->list, head);

	return 0;
error:
	kfree(resume_cfg->arg);
	kfree(resume_cfg);
	return ret;
}

static void
wl_android_iolist_resume(struct net_device *dev, struct list_head *head)
{
	struct io_cfg *config;
	struct list_head *cur, *q;
	s32 ret = 0;

	list_for_each_safe(cur, q, head) {
		config = list_entry(cur, struct io_cfg, list);
		if (config->iovar) {
			if (!ret)
				ret = wldev_iovar_setint(dev, config->iovar,
					config->param);
		} else {
			if (!ret)
				ret = wldev_ioctl(dev, config->ioctl + 1,
					config->arg, config->len, true);
			if (config->ioctl + 1 == WLC_SET_PM)
				wl_cfg80211_update_power_mode(dev);
			kfree(config->arg);
		}
		list_del(cur);
		kfree(config);
	}
}
#ifdef WL11ULB
static int
wl_android_set_ulb_mode(struct net_device *dev, char *command, int total_len)
{
	int mode = 0;

	DHD_INFO(("set ulb mode (%s) \n", command));
	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}
	return wl_cfg80211_set_ulb_mode(dev, mode);
}
static int
wl_android_set_ulb_bw(struct net_device *dev, char *command, int total_len)
{
	int bw = 0;
	u8 *pos;
	char *ifname = NULL;
	DHD_INFO(("set ulb bw (%s) \n", command));

	/*
	 * For sta/ap: IFNAME=<ifname> DRIVER ULB_BW <bw> ifname
	 * For p2p:    IFNAME=wlan0 DRIVER ULB_BW <bw> p2p-dev-wlan0
	 */
	if (total_len < strlen(CMD_ULB_BW) + 2)
		return -EINVAL;

	pos = command + strlen(CMD_ULB_BW) + 1;
	bw = bcm_atoi(pos);

	if ((strlen(pos) >= 5)) {
		ifname = pos + 2;
	}

	DHD_INFO(("[ULB] ifname:%s ulb_bw:%d \n", ifname, bw));
	return wl_cfg80211_set_ulb_bw(dev, bw, ifname);
}
#endif /* WL11ULB */
static int
wl_android_set_miracast(struct net_device *dev, char *command, int total_len)
{
	int mode, val = 0;
	int ret = 0;
	struct io_cfg config;

	if (sscanf(command, "%*s %d", &mode) != 1) {
		DHD_ERROR(("%s: Failed to get Parameter\n", __FUNCTION__));
		return -1;
	}

	DHD_INFO(("%s: enter miracast mode %d\n", __FUNCTION__, mode));

	if (miracast_cur_mode == mode) {
		return 0;
	}

	wl_android_iolist_resume(dev, &miracast_resume_list);
	miracast_cur_mode = MIRACAST_MODE_OFF;

	switch (mode) {
	case MIRACAST_MODE_SOURCE:
		/* setting mchan_algo to platform specific value */
		config.iovar = "mchan_algo";

		ret = wldev_ioctl(dev, WLC_GET_BCNPRD, &val, sizeof(int), false);
		if (!ret && val > 100) {
			config.param = 0;
			DHD_ERROR(("%s: Connected station's beacon interval: "
				"%d and set mchan_algo to %d \n",
				__FUNCTION__, val, config.param));
		} else {
			config.param = MIRACAST_MCHAN_ALGO;
		}
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret) {
			goto resume;
		}

		/* setting mchan_bw to platform specific value */
		config.iovar = "mchan_bw";
		config.param = MIRACAST_MCHAN_BW;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret) {
			goto resume;
		}

		/* setting apmdu to platform specific value */
		config.iovar = "ampdu_mpdu";
		config.param = MIRACAST_AMPDU_SIZE;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret) {
			goto resume;
		}
		/* FALLTROUGH */
		/* Source mode shares most configurations with sink mode.
		 * Fall through here to avoid code duplication
		 */
	case MIRACAST_MODE_SINK:
		/* disable internal roaming */
		config.iovar = "roam_off";
		config.param = 1;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret) {
			goto resume;
		}
#ifdef CUSTOMER_HW10
		/* [CSP#812738] Change scan engine parameters to reduce scan time
		 * and guarantee more times to mirroring.
		 */
		val = 10;
		config.iovar = NULL;
		config.ioctl = WLC_GET_SCAN_CHANNEL_TIME;
		config.arg = &val;
		config.len = sizeof(int);
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;

		val = 180;
		config.iovar = NULL;
		config.ioctl = WLC_GET_SCAN_HOME_TIME;
		config.arg = &val;
		config.len = sizeof(int);
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		if (ret)
			goto resume;

#if defined(BCM4339_CHIP)
		config.iovar = "phy_watchdog";
		config.param = 0;
		ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
		DHD_INFO(("%s: do iovar cmd=%s (ret=%d)\n", __FUNCTION__, config.iovar, ret));
#endif
#else

		/* tunr off pm */
		ret = wldev_ioctl(dev, WLC_GET_PM, &val, sizeof(val), false);
		if (ret) {
			goto resume;
		}

		if (val != PM_OFF) {
			val = PM_OFF;
			config.iovar = NULL;
			config.ioctl = WLC_GET_PM;
			config.arg = &val;
			config.len = sizeof(int);
			ret = wl_android_iolist_add(dev, &miracast_resume_list, &config);
			if (ret) {
				goto resume;
			}
		}
#endif /* CUSTOMER_HW10 */
		break;
	case MIRACAST_MODE_OFF:
	default:
		break;
	}
	miracast_cur_mode = mode;

	return 0;

resume:
	DHD_ERROR(("%s: turnoff miracast mode because of err%d\n", __FUNCTION__, ret));
	wl_android_iolist_resume(dev, &miracast_resume_list);
	return ret;
}

#define NETLINK_OXYGEN     30
#define AIBSS_BEACON_TIMEOUT	10

static struct sock *nl_sk = NULL;

static void wl_netlink_recv(struct sk_buff *skb)
{
	WL_ERR(("netlink_recv called\n"));
}

static int wl_netlink_init(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct netlink_kernel_cfg cfg = {
		.input	= wl_netlink_recv,
	};
#endif

	if (nl_sk != NULL) {
		WL_ERR(("nl_sk already exist\n"));
		return BCME_ERROR;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
	nl_sk = netlink_kernel_create(&init_net, NETLINK_OXYGEN,
		0, wl_netlink_recv, NULL, THIS_MODULE);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0))
	nl_sk = netlink_kernel_create(&init_net, NETLINK_OXYGEN, THIS_MODULE, &cfg);
#else
	nl_sk = netlink_kernel_create(&init_net, NETLINK_OXYGEN, &cfg);
#endif

	if (nl_sk == NULL) {
		WL_ERR(("nl_sk is not ready\n"));
		return BCME_ERROR;
	}

	return BCME_OK;
}

static void wl_netlink_deinit(void)
{
	if (nl_sk) {
		netlink_kernel_release(nl_sk);
		nl_sk = NULL;
	}
}

s32
wl_netlink_send_msg(int pid, int type, int seq, void *data, size_t size)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	int ret = -1;

	if (nl_sk == NULL) {
		WL_ERR(("nl_sk was not initialized\n"));
		goto nlmsg_failure;
	}

	skb = alloc_skb(NLMSG_SPACE(size), GFP_ATOMIC);
	if (skb == NULL) {
		WL_ERR(("failed to allocate memory\n"));
		goto nlmsg_failure;
	}

	nlh = nlmsg_put(skb, 0, 0, 0, size, 0);
	if (nlh == NULL) {
		WL_ERR(("failed to build nlmsg, skb_tailroom:%d, nlmsg_total_size:%d\n",
			skb_tailroom(skb), nlmsg_total_size(size)));
		dev_kfree_skb(skb);
		goto nlmsg_failure;
	}

	memcpy(nlmsg_data(nlh), data, size);
	nlh->nlmsg_seq = seq;
	nlh->nlmsg_type = type;

	/* netlink_unicast() takes ownership of the skb and frees it itself. */
	ret = netlink_unicast(nl_sk, skb, pid, 0);
	WL_DBG(("netlink_unicast() pid=%d, ret=%d\n", pid, ret));

nlmsg_failure:
	return ret;
}

#ifdef WLAIBSS
static int wl_android_set_ibss_txfail_event(struct net_device *dev, char *command, int total_len)
{
	int err = 0;
	int retry = 0;
	int pid = 0;
	aibss_txfail_config_t txfail_config = {0, 0, 0, 0};
	char smbuf[WLC_IOCTL_SMLEN];

	if (sscanf(command, CMD_SETIBSSTXFAILEVENT " %d %d", &retry, &pid) <= 0) {
		WL_ERR(("Failed to get Parameter from : %s\n", command));
		return -1;
	}

	/* set pid, and if the event was happened, let's send a notification through netlink */
	wl_cfg80211_set_txfail_pid(pid);
#ifdef CUSTOMER_HW4
	/* using same pid for RMC, AIBSS shares same pid with RMC and it is set once */
	wl_cfg80211_set_rmc_pid(pid);
#endif

	/* If retry value is 0, it disables the functionality for TX Fail. */
	if (retry > 0) {
		txfail_config.max_tx_retry = retry;
		txfail_config.bcn_timeout = 0;	/* 0 : disable tx fail from beacon */
	}
	txfail_config.version = AIBSS_TXFAIL_CONFIG_VER_0;
	txfail_config.len = sizeof(txfail_config);

	err = wldev_iovar_setbuf(dev, "aibss_txfail_config", (void *) &txfail_config,
		sizeof(aibss_txfail_config_t), smbuf, WLC_IOCTL_SMLEN, NULL);
	WL_DBG(("retry=%d, pid=%d, err=%d\n", retry, pid, err));

	return ((err == 0)?total_len:err);
}

static int wl_android_get_ibss_peer_info(struct net_device *dev, char *command,
	int total_len, bool bAll)
{
	int error;
	int bytes_written = 0;
	void *buf = NULL;
	bss_peer_list_info_t peer_list_info;
	bss_peer_info_t *peer_info;
	int i;
	bool found = false;
	struct ether_addr mac_ea;

	WL_DBG(("get ibss peer info(%s)\n", bAll?"true":"false"));

	if (!bAll) {
		if (sscanf (command, "GETIBSSPEERINFO %02x:%02x:%02x:%02x:%02x:%02x",
			(unsigned int *)&mac_ea.octet[0], (unsigned int *)&mac_ea.octet[1],
			(unsigned int *)&mac_ea.octet[2], (unsigned int *)&mac_ea.octet[3],
			(unsigned int *)&mac_ea.octet[4], (unsigned int *)&mac_ea.octet[5]) != 6) {
			WL_DBG(("invalid MAC address\n"));
			return -1;
		}
	}

	if ((buf = kmalloc(WLC_IOCTL_MAXLEN, GFP_KERNEL)) == NULL) {
		WL_ERR(("kmalloc failed\n"));
		return -1;
	}

	error = wldev_iovar_getbuf(dev, "bss_peer_info", NULL, 0, buf, WLC_IOCTL_MAXLEN, NULL);
	if (unlikely(error)) {
		WL_ERR(("could not get ibss peer info (%d)\n", error));
		kfree(buf);
		return -1;
	}

	memcpy(&peer_list_info, buf, sizeof(peer_list_info));
	peer_list_info.version = htod16(peer_list_info.version);
	peer_list_info.bss_peer_info_len = htod16(peer_list_info.bss_peer_info_len);
	peer_list_info.count = htod32(peer_list_info.count);

	WL_DBG(("ver:%d, len:%d, count:%d\n", peer_list_info.version,
		peer_list_info.bss_peer_info_len, peer_list_info.count));

	if (peer_list_info.count > 0) {
		if (bAll)
			bytes_written += snprintf(&command[bytes_written], total_len, "%u ",
				peer_list_info.count);

		peer_info = (bss_peer_info_t *) ((void *)buf + BSS_PEER_LIST_INFO_FIXED_LEN);


		for (i = 0; i < peer_list_info.count; i++) {

			WL_DBG(("index:%d rssi:%d, tx:%u, rx:%u\n", i, peer_info->rssi,
				peer_info->tx_rate, peer_info->rx_rate));

			if (!bAll &&
				memcmp(&mac_ea, &peer_info->ea, sizeof(struct ether_addr)) == 0) {
				found = true;
			}

			if (bAll || found) {
				bytes_written += snprintf(&command[bytes_written], total_len,
					MACF, ETHER_TO_MACF(peer_info->ea));
				bytes_written += snprintf(&command[bytes_written], total_len,
					" %u %d ", peer_info->tx_rate/1000, peer_info->rssi);
			}

			if (found)
				break;

			peer_info = (bss_peer_info_t *)((void *)peer_info+sizeof(bss_peer_info_t));
		}
	}
	else {
		WL_ERR(("could not get ibss peer info : no item\n"));
	}
	bytes_written += snprintf(&command[bytes_written], total_len, "%s", "\0");

	WL_DBG(("command(%u):%s\n", total_len, command));
	WL_DBG(("bytes_written:%d\n", bytes_written));

	kfree(buf);
	return bytes_written;
}

int wl_android_set_ibss_routetable(struct net_device *dev, char *command, int total_len)
{

	char *pcmd = command;
	char *str = NULL;

	ibss_route_tbl_t *route_tbl = NULL;
	char *ioctl_buf = NULL;
	u16 kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	s32 err = BCME_OK;
	uint32 route_tbl_len;
	uint32 entries;
	char *endptr;
	uint32 i = 0;
	struct ipv4_addr  dipaddr;
	struct ether_addr ea;

	route_tbl_len = sizeof(ibss_route_tbl_t) +
		(MAX_IBSS_ROUTE_TBL_ENTRY - 1) * sizeof(ibss_route_entry_t);
	route_tbl = (ibss_route_tbl_t *)kzalloc(route_tbl_len, kflags);
	if (!route_tbl) {
		WL_ERR(("Route TBL alloc failed\n"));
		return -ENOMEM;
	}
	ioctl_buf = kzalloc(WLC_IOCTL_MEDLEN, GFP_KERNEL);
	if (!ioctl_buf) {
		WL_ERR(("ioctl memory alloc failed\n"));
		if (route_tbl) {
			kfree(route_tbl);
		}
		return -ENOMEM;
	}
	memset(ioctl_buf, 0, WLC_IOCTL_MEDLEN);

	/* drop command */
	str = bcmstrtok(&pcmd, " ", NULL);

	/* get count */
	str = bcmstrtok(&pcmd, " ",  NULL);
	if (!str) {
		WL_ERR(("Invalid number parameter %s\n", str));
		err = -EINVAL;
		goto exit;
	}
	entries = bcm_strtoul(str, &endptr, 0);
	if (*endptr != '\0') {
		WL_ERR(("Invalid number parameter %s\n", str));
		err = -EINVAL;
		goto exit;
	}
	WL_INFORM(("Routing table count:%d\n", entries));
	route_tbl->num_entry = entries;

	for (i = 0; i < entries; i++) {
		str = bcmstrtok(&pcmd, " ", NULL);
		if (!str || !bcm_atoipv4(str, &dipaddr)) {
			WL_ERR(("Invalid ip string %s\n", str));
			err = -EINVAL;
			goto exit;
		}


		str = bcmstrtok(&pcmd, " ", NULL);
		if (!str || !bcm_ether_atoe(str, &ea)) {
			WL_ERR(("Invalid ethernet string %s\n", str));
			err = -EINVAL;
			goto exit;
		}
		bcopy(&dipaddr, &route_tbl->route_entry[i].ipv4_addr, IPV4_ADDR_LEN);
		bcopy(&ea, &route_tbl->route_entry[i].nexthop, ETHER_ADDR_LEN);
	}

	route_tbl_len = sizeof(ibss_route_tbl_t) +
		((!entries?0:(entries - 1)) * sizeof(ibss_route_entry_t));
	err = wldev_iovar_setbuf(dev, "ibss_route_tbl",
		route_tbl, route_tbl_len, ioctl_buf, WLC_IOCTL_MEDLEN, NULL);
	if (err != BCME_OK) {
		WL_ERR(("Fail to set iovar %d\n", err));
		err = -EINVAL;
	}

exit:
	if (route_tbl)
		kfree(route_tbl);
	if (ioctl_buf)
		kfree(ioctl_buf);
	return err;

}


int
wl_android_set_ibss_ampdu(struct net_device *dev, char *command, int total_len)
{
	char *pcmd = command;
	char *str = NULL, *endptr = NULL;
	struct ampdu_aggr aggr;
	char smbuf[WLC_IOCTL_SMLEN];
	int idx;
	int err = 0;
	int wme_AC2PRIO[AC_COUNT][2] = {
		{PRIO_8021D_VO, PRIO_8021D_NC},		/* AC_VO - 3 */
		{PRIO_8021D_CL, PRIO_8021D_VI},		/* AC_VI - 2 */
		{PRIO_8021D_BK, PRIO_8021D_NONE},	/* AC_BK - 1 */
		{PRIO_8021D_BE, PRIO_8021D_EE}};	/* AC_BE - 0 */

	WL_DBG(("set ibss ampdu:%s\n", command));

	memset(&aggr, 0, sizeof(aggr));
	/* Cofigure all priorities */
	aggr.conf_TID_bmap = NBITMASK(NUMPRIO);

	/* acquire parameters */
	/* drop command */
	str = bcmstrtok(&pcmd, " ", NULL);

	for (idx = 0; idx < AC_COUNT; idx++) {
		bool on;
		str = bcmstrtok(&pcmd, " ", NULL);
		if (!str) {
			WL_ERR(("Invalid parameter : %s\n", pcmd));
			return -EINVAL;
		}
		on = bcm_strtoul(str, &endptr, 0) ? TRUE : FALSE;
		if (*endptr != '\0') {
			WL_ERR(("Invalid number format %s\n", str));
			return -EINVAL;
		}
		if (on) {
			setbit(&aggr.enab_TID_bmap, wme_AC2PRIO[idx][0]);
			setbit(&aggr.enab_TID_bmap, wme_AC2PRIO[idx][1]);
		}
	}

	err = wldev_iovar_setbuf(dev, "ampdu_txaggr", (void *)&aggr,
	sizeof(aggr), smbuf, WLC_IOCTL_SMLEN, NULL);

	return ((err == 0) ? total_len : err);
}

int wl_android_set_ibss_antenna(struct net_device *dev, char *command, int total_len)
{
	char *pcmd = command;
	char *str = NULL;
	int txchain, rxchain;
	int err = 0;

	WL_DBG(("set ibss antenna:%s\n", command));

	/* acquire parameters */
	/* drop command */
	str = bcmstrtok(&pcmd, " ", NULL);

	/* TX chain */
	str = bcmstrtok(&pcmd, " ", NULL);
	if (!str) {
		WL_ERR(("Invalid parameter : %s\n", pcmd));
		return -EINVAL;
	}
	txchain = bcm_atoi(str);

	/* RX chain */
	str = bcmstrtok(&pcmd, " ", NULL);
	if (!str) {
		WL_ERR(("Invalid parameter : %s\n", pcmd));
		return -EINVAL;
	}
	rxchain = bcm_atoi(str);

	err = wldev_iovar_setint(dev, "txchain", txchain);
	if (err != 0)
		return err;
	err = wldev_iovar_setint(dev, "rxchain", rxchain);
	return ((err == 0)?total_len:err);
}
#endif /* WLAIBSS */

int wl_keep_alive_set(struct net_device *dev, char* extra, int total_len)
{
	wl_mkeep_alive_pkt_t	mkeep_alive_pkt;
	int ret;
	uint period_msec = 0;
	char *buf;

	if (extra == NULL)
	{
		 DHD_ERROR(("%s: extra is NULL\n", __FUNCTION__));
		 return -1;
	}
	if (sscanf(extra, "%d", &period_msec) != 1)
	{
		 DHD_ERROR(("%s: sscanf error. check period_msec value\n", __FUNCTION__));
		 return -EINVAL;
	}
	DHD_ERROR(("%s: period_msec is %d\n", __FUNCTION__, period_msec));

	memset(&mkeep_alive_pkt, 0, sizeof(wl_mkeep_alive_pkt_t));

	mkeep_alive_pkt.period_msec = period_msec;
	mkeep_alive_pkt.version = htod16(WL_MKEEP_ALIVE_VERSION);
	mkeep_alive_pkt.length = htod16(WL_MKEEP_ALIVE_FIXED_LEN);

	/* Setup keep alive zero for null packet generation */
	mkeep_alive_pkt.keep_alive_id = 0;
	mkeep_alive_pkt.len_bytes = 0;

	buf = kmalloc(WLC_IOCTL_SMLEN, GFP_KERNEL);
	if (!buf) {
		DHD_ERROR(("%s: buffer alloc failed\n", __FUNCTION__));
		return BCME_NOMEM;
	}

	ret = wldev_iovar_setbuf(dev, "mkeep_alive", (char *)&mkeep_alive_pkt,
		WL_MKEEP_ALIVE_FIXED_LEN, buf, WLC_IOCTL_SMLEN, NULL);
	if (ret < 0)
		DHD_ERROR(("%s:keep_alive set failed:%d\n", __FUNCTION__, ret));
	else
		DHD_TRACE(("%s:keep_alive set ok\n", __FUNCTION__));
	kfree(buf);
	return ret;
}

static const char *
get_string_by_separator(char *result, int result_len, const char *src, char separator)
{
	char *end = result + result_len - 1;
	while ((result != end) && (*src != separator) && (*src)) {
		*result++ = *src++;
	}
	*result = 0;
	if (*src == separator) {
		++src;
	}
	return src;
}

int
wl_android_set_roam_offload_bssid_list(struct net_device *dev, const char *cmd)
{
	char sbuf[32];
	int i, cnt, size, err, ioctl_buf_len;
	roamoffl_bssid_list_t *bssid_list;
	const char *str = cmd;
	char *ioctl_buf;

	str = get_string_by_separator(sbuf, 32, str, ',');
	cnt = bcm_atoi(sbuf);
	if (cnt < 0 || cnt >= MAX_ROAMOFFL_BSSID_NUM) {
		WL_ERR(("BSSID List count (%d) invalid\n", cnt));
		return -EINVAL;
	}
	size = sizeof(int32) + sizeof(struct ether_addr) * cnt;
	WL_ERR(("ROAM OFFLOAD BSSID LIST %d BSSIDs, size %d\n", cnt, size));
	bssid_list = kmalloc(size, GFP_KERNEL);
	if (bssid_list == NULL) {
		WL_ERR(("%s: memory alloc for bssid list(%d) failed\n",
			__FUNCTION__, size));
		return -ENOMEM;
	}
	ioctl_buf_len = size + 64;
	ioctl_buf = kmalloc(ioctl_buf_len, GFP_KERNEL);
	if (ioctl_buf == NULL) {
		WL_ERR(("%s: memory alloc for ioctl_buf(%d) failed\n",
			__FUNCTION__, ioctl_buf_len));
		kfree(bssid_list);
		return -ENOMEM;
	}

	for (i = 0; i < cnt; i++) {
		str = get_string_by_separator(sbuf, 32, str, ',');
		bcm_ether_atoe(sbuf, &bssid_list->bssid[i]);
	}

	bssid_list->cnt = (int32)cnt;
	err = wldev_iovar_setbuf(dev, "roamoffl_bssid_list",
			bssid_list, size, ioctl_buf, ioctl_buf_len, NULL);
	kfree(bssid_list);
	kfree(ioctl_buf);

	return err;
}

#ifdef P2PRESP_WFDIE_SRC
static int wl_android_get_wfdie_resp(struct net_device *dev, char *command, int total_len)
{
	int error = 0;
	int bytes_written = 0;
	int only_resp_wfdsrc = 0;

	error = wldev_iovar_getint(dev, "p2p_only_resp_wfdsrc", &only_resp_wfdsrc);
	if (error) {
		DHD_ERROR(("%s: Failed to get the mode for only_resp_wfdsrc, error = %d\n",
			__FUNCTION__, error));
		return -1;
	}

	bytes_written = snprintf(command, total_len, "%s %d",
		CMD_P2P_GET_WFDIE_RESP, only_resp_wfdsrc);

	return bytes_written;
}

static int wl_android_set_wfdie_resp(struct net_device *dev, int only_resp_wfdsrc)
{
	int error = 0;

	error = wldev_iovar_setint(dev, "p2p_only_resp_wfdsrc", only_resp_wfdsrc);
	if (error) {
		DHD_ERROR(("%s: Failed to set only_resp_wfdsrc %d, error = %d\n",
			__FUNCTION__, only_resp_wfdsrc, error));
		return -1;
	}

	return 0;
}
#endif /* P2PRESP_WFDIE_SRC */

#ifdef BT_WIFI_HANDOVER
static int
wl_tbow_teardown(struct net_device *dev, char *command, int total_len)
{
	int err = BCME_OK;
	char buf[WLC_IOCTL_SMLEN];
	tbow_setup_netinfo_t netinfo;
	memset(&netinfo, 0, sizeof(netinfo));
	netinfo.opmode = TBOW_HO_MODE_TEARDOWN;

	err = wldev_iovar_setbuf_bsscfg(dev, "tbow_doho", &netinfo,
			sizeof(tbow_setup_netinfo_t), buf, WLC_IOCTL_SMLEN, 0, NULL);
	if (err < 0) {
		WL_ERR(("tbow_doho iovar error %d\n", err));
			return err;
	}
	return err;
}
#endif /* BT_WIFI_HANOVER */

#ifdef SET_RPS_CPUS
static int
wl_android_set_rps_cpus(struct net_device *dev, char *command, int total_len)
{
	int error, enable;

	enable = command[strlen(CMD_RPSMODE) + 1] - '0';
	error = dhd_rps_cpus_enable(dev, enable);

#if defined(DHDTCPACK_SUPPRESS) && defined(BCMPCIE) && defined(WL_CFG80211)
	if (!error) {
		void *dhdp = wl_cfg80211_get_dhdp();
		if (enable) {
			DHD_TRACE(("%s : set ack suppress. TCPACK_SUP_HOLD.\n", __FUNCTION__));
			dhd_tcpack_suppress_set(dhdp, TCPACK_SUP_HOLD);
		} else {
			DHD_TRACE(("%s : clear ack suppress.\n", __FUNCTION__));
			dhd_tcpack_suppress_set(dhdp, TCPACK_SUP_OFF);
		}
	}
#endif /* DHDTCPACK_SUPPRESS && BCMPCIE && WL_CFG80211 */

	return error;
}
#endif /* SET_RPS_CPUS */
int wl_android_priv_cmd(struct net_device *net, struct ifreq *ifr, int cmd)
{
#define PRIVATE_COMMAND_MAX_LEN	8192
	int ret = 0;
	char *command = NULL;
	int bytes_written = 0;
	android_wifi_priv_cmd priv_cmd;

	net_os_wake_lock(net);

	if (!ifr->ifr_data) {
		ret = -EINVAL;
		goto exit;
	}

#ifdef CONFIG_COMPAT
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
	if (in_compat_syscall()) {
#else
	if (is_compat_task()) {
#endif
		compat_android_wifi_priv_cmd compat_priv_cmd;
		if (copy_from_user(&compat_priv_cmd, ifr->ifr_data,
			sizeof(compat_android_wifi_priv_cmd))) {
			ret = -EFAULT;
			goto exit;

		}
		priv_cmd.buf = compat_ptr(compat_priv_cmd.buf);
		priv_cmd.used_len = compat_priv_cmd.used_len;
		priv_cmd.total_len = compat_priv_cmd.total_len;
	} else
#endif /* CONFIG_COMPAT */
	{
		if (copy_from_user(&priv_cmd, ifr->ifr_data, sizeof(android_wifi_priv_cmd))) {
			ret = -EFAULT;
			goto exit;
		}
	}
	if ((priv_cmd.total_len > PRIVATE_COMMAND_MAX_LEN) || (priv_cmd.total_len < 0)) {
		DHD_ERROR(("%s: too long priavte command\n", __FUNCTION__));
		ret = -EINVAL;
		goto exit;
	}
	command = kmalloc((priv_cmd.total_len + 1), GFP_KERNEL);
	if (!command)
	{
		DHD_ERROR(("%s: failed to allocate memory\n", __FUNCTION__));
		ret = -ENOMEM;
		goto exit;
	}
	if (copy_from_user(command, priv_cmd.buf, priv_cmd.total_len)) {
		ret = -EFAULT;
		goto exit;
	}
	command[priv_cmd.total_len] = '\0';

	DHD_INFO(("%s: Android private cmd \"%s\" on %s\n", __FUNCTION__, command, ifr->ifr_name));

	bytes_written = wl_handle_private_cmd(net, command, priv_cmd.total_len);
	if (bytes_written >= 0) {
		if ((bytes_written == 0) && (priv_cmd.total_len > 0)) {
			command[0] = '\0';
		}
		if (bytes_written >= priv_cmd.total_len) {
			DHD_ERROR(("%s: bytes_written = %d\n", __FUNCTION__, bytes_written));
			bytes_written = priv_cmd.total_len;
		} else {
			bytes_written++;
		}
		priv_cmd.used_len = bytes_written;
		if (copy_to_user(priv_cmd.buf, command, bytes_written)) {
			DHD_ERROR(("%s: failed to copy data to user buffer\n", __FUNCTION__));
			ret = -EFAULT;
		}
	}
	else {
		/* Propagate the error */
		ret = bytes_written;
	}

exit:
	net_os_wake_unlock(net);
	if (command) {
		kfree(command);
	}

	return ret;
}

int
wl_handle_private_cmd(struct net_device *net, char *command, u32 cmd_len)
{
	int bytes_written = 0;
	android_wifi_priv_cmd priv_cmd = {0};

	priv_cmd.total_len = cmd_len;

	if (strnicmp(command, CMD_START, strlen(CMD_START)) == 0) {
		DHD_INFO(("%s, Received regular START command\n", __FUNCTION__));
#ifdef SUPPORT_DEEP_SLEEP
		trigger_deep_sleep = 1;
#else
		bytes_written = wl_android_wifi_on(net);
#endif /* SUPPORT_DEEP_SLEEP */
	}
	else if (strnicmp(command, CMD_SETFWPATH, strlen(CMD_SETFWPATH)) == 0) {
		bytes_written = wl_android_set_fwpath(net, command, priv_cmd.total_len);
	}

	if (!g_wifi_on) {
		DHD_ERROR(("%s: Ignore private cmd \"%s\" - iface is down\n",
			__FUNCTION__, command));
		return 0;
	}

	if (strnicmp(command, CMD_STOP, strlen(CMD_STOP)) == 0) {
#ifdef SUPPORT_DEEP_SLEEP
		trigger_deep_sleep = 1;
#else
		bytes_written = wl_android_wifi_off(net, FALSE);
#endif /* SUPPORT_DEEP_SLEEP */
	}
	else if (strnicmp(command, CMD_SCAN_ACTIVE, strlen(CMD_SCAN_ACTIVE)) == 0) {
		wl_cfg80211_set_passive_scan(net, command);
	}
	else if (strnicmp(command, CMD_SCAN_PASSIVE, strlen(CMD_SCAN_PASSIVE)) == 0) {
		wl_cfg80211_set_passive_scan(net, command);
	}
	else if (strnicmp(command, CMD_RSSI, strlen(CMD_RSSI)) == 0) {
		bytes_written = wl_android_get_rssi(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_LINKSPEED, strlen(CMD_LINKSPEED)) == 0) {
		bytes_written = wl_android_get_link_speed(net, command, priv_cmd.total_len);
	}
#ifdef PKT_FILTER_SUPPORT
	else if (strnicmp(command, CMD_RXFILTER_START, strlen(CMD_RXFILTER_START)) == 0) {
		bytes_written = net_os_enable_packet_filter(net, 1);
	}
	else if (strnicmp(command, CMD_RXFILTER_STOP, strlen(CMD_RXFILTER_STOP)) == 0) {
		bytes_written = net_os_enable_packet_filter(net, 0);
	}
	else if (strnicmp(command, CMD_RXFILTER_ADD, strlen(CMD_RXFILTER_ADD)) == 0) {
		int filter_num = *(command + strlen(CMD_RXFILTER_ADD) + 1) - '0';
		bytes_written = net_os_rxfilter_add_remove(net, TRUE, filter_num);
	}
	else if (strnicmp(command, CMD_RXFILTER_REMOVE, strlen(CMD_RXFILTER_REMOVE)) == 0) {
		int filter_num = *(command + strlen(CMD_RXFILTER_REMOVE) + 1) - '0';
		bytes_written = net_os_rxfilter_add_remove(net, FALSE, filter_num);
	}
#endif /* PKT_FILTER_SUPPORT */
	else if (strnicmp(command, CMD_BTCOEXSCAN_START, strlen(CMD_BTCOEXSCAN_START)) == 0) {
		/* TBD: BTCOEXSCAN-START */
	}
	else if (strnicmp(command, CMD_BTCOEXSCAN_STOP, strlen(CMD_BTCOEXSCAN_STOP)) == 0) {
		/* TBD: BTCOEXSCAN-STOP */
	}
	else if (strnicmp(command, CMD_BTCOEXMODE, strlen(CMD_BTCOEXMODE)) == 0) {
#ifdef WL_CFG80211
		void *dhdp = wl_cfg80211_get_dhdp();
		bytes_written = wl_cfg80211_set_btcoex_dhcp(net, dhdp, command);
#else
#ifdef PKT_FILTER_SUPPORT
		uint mode = *(command + strlen(CMD_BTCOEXMODE) + 1) - '0';

		if (mode == 1)
			net_os_enable_packet_filter(net, 0); /* DHCP starts */
		else
			net_os_enable_packet_filter(net, 1); /* DHCP ends */
#endif /* PKT_FILTER_SUPPORT */
#endif /* WL_CFG80211 */
	}
	else if (strnicmp(command, CMD_SETSUSPENDOPT, strlen(CMD_SETSUSPENDOPT)) == 0) {
		bytes_written = wl_android_set_suspendopt(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSUSPENDMODE, strlen(CMD_SETSUSPENDMODE)) == 0) {
		bytes_written = wl_android_set_suspendmode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETBAND, strlen(CMD_SETBAND)) == 0) {
		uint band = *(command + strlen(CMD_SETBAND) + 1) - '0';
#ifdef WL_HOST_BAND_MGMT
		s32 ret = 0;
		if ((ret = wl_cfg80211_set_band(net, band)) < 0) {
			if (ret == BCME_UNSUPPORTED) {
				/* If roam_var is unsupported, fallback to the original method */
				WL_ERR(("WL_HOST_BAND_MGMT defined, "
					"but roam_band iovar unsupported in the firmware\n"));
			} else {
				bytes_written = -1;
			}
		}
		if (((ret == 0) && (band == WLC_BAND_AUTO)) || (ret == BCME_UNSUPPORTED)) {
			/* Apply if roam_band iovar is not supported or band setting is AUTO */
			bytes_written = wldev_set_band(net, band);
		}
#else
		bytes_written = wldev_set_band(net, band);
#endif /* WL_HOST_BAND_MGMT */
#if defined(CUSTOMER_HW4) && defined(ROAM_CHANNEL_CACHE)
		wl_update_roamscan_cache_by_band(net, band);
#endif
	}
	else if (strnicmp(command, CMD_GETBAND, strlen(CMD_GETBAND)) == 0) {
		bytes_written = wl_android_get_band(net, command, priv_cmd.total_len);
	}
#ifdef WL_CFG80211
#ifndef CUSTOMER_SET_COUNTRY
	/* CUSTOMER_SET_COUNTRY feature is define for only GGSM model */
	else if (strnicmp(command, CMD_COUNTRY, strlen(CMD_COUNTRY)) == 0) {
		/*
		 * Usage examples:
		 * DRIVER COUNTRY US
		 * DRIVER COUNTRY US/7
		 */
		char *country_code = command + strlen(CMD_COUNTRY) + 1;
		char *rev_info_delim = country_code + 2; /* 2 bytes of country code */
		int revinfo = -1;
		if ((rev_info_delim) &&
			(strnicmp(rev_info_delim, CMD_COUNTRY_DELIMITER,
			strlen(CMD_COUNTRY_DELIMITER)) == 0) &&
			(rev_info_delim + 1)) {
			revinfo  = bcm_atoi(rev_info_delim + 1);
		}
		bytes_written = wldev_set_country(net, country_code, true, true, revinfo);
	}
#endif /* CUSTOMER_SET_COUNTRY */
#endif /* WL_CFG80211 */
	else if (strnicmp(command, CMD_SET_CSA, strlen(CMD_SET_CSA)) == 0) {
		bytes_written = wl_android_set_csa(net, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_80211_MODE, strlen(CMD_80211_MODE)) == 0) {
		bytes_written = wl_android_get_80211_mode(net, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_CHANSPEC, strlen(CMD_CHANSPEC)) == 0) {
		bytes_written = wl_android_get_chanspec(net, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_DATARATE, strlen(CMD_DATARATE)) == 0) {
		bytes_written = wl_android_get_datarate(net, command, priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ASSOC_CLIENTS,	strlen(CMD_ASSOC_CLIENTS)) == 0) {
		bytes_written = wl_android_get_assoclist(net, command, priv_cmd.total_len);
	}

#ifdef CUSTOMER_HW4
#ifdef ROAM_API
	else if (strnicmp(command, CMD_ROAMTRIGGER_SET,
		strlen(CMD_ROAMTRIGGER_SET)) == 0) {
		bytes_written = wl_android_set_roam_trigger(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMTRIGGER_GET,
		strlen(CMD_ROAMTRIGGER_GET)) == 0) {
		bytes_written = wl_android_get_roam_trigger(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMDELTA_SET,
		strlen(CMD_ROAMDELTA_SET)) == 0) {
		bytes_written = wl_android_set_roam_delta(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMDELTA_GET,
		strlen(CMD_ROAMDELTA_GET)) == 0) {
		bytes_written = wl_android_get_roam_delta(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMSCANPERIOD_SET,
		strlen(CMD_ROAMSCANPERIOD_SET)) == 0) {
		bytes_written = wl_android_set_roam_scan_period(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_ROAMSCANPERIOD_GET,
		strlen(CMD_ROAMSCANPERIOD_GET)) == 0) {
		bytes_written = wl_android_get_roam_scan_period(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_FULLROAMSCANPERIOD_SET,
		strlen(CMD_FULLROAMSCANPERIOD_SET)) == 0) {
		bytes_written = wl_android_set_full_roam_scan_period(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_FULLROAMSCANPERIOD_GET,
		strlen(CMD_FULLROAMSCANPERIOD_GET)) == 0) {
		bytes_written = wl_android_get_full_roam_scan_period(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_COUNTRYREV_SET,
		strlen(CMD_COUNTRYREV_SET)) == 0) {
		bytes_written = wl_android_set_country_rev(net, command,
		priv_cmd.total_len);
	} else if (strnicmp(command, CMD_COUNTRYREV_GET,
		strlen(CMD_COUNTRYREV_GET)) == 0) {
		bytes_written = wl_android_get_country_rev(net, command,
		priv_cmd.total_len);
	}
#endif /* ROAM_API */
#ifdef WES_SUPPORT
	else if (strnicmp(command, CMD_GETROAMSCANCONTROL, strlen(CMD_GETROAMSCANCONTROL)) == 0) {
		bytes_written = wl_android_get_roam_scan_control(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETROAMSCANCONTROL, strlen(CMD_SETROAMSCANCONTROL)) == 0) {
		bytes_written = wl_android_set_roam_scan_control(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETROAMSCANCHANNELS, strlen(CMD_GETROAMSCANCHANNELS)) == 0) {
		bytes_written = wl_android_get_roam_scan_channels(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETROAMSCANCHANNELS, strlen(CMD_SETROAMSCANCHANNELS)) == 0) {
		bytes_written = wl_android_set_roam_scan_channels(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SENDACTIONFRAME, strlen(CMD_SENDACTIONFRAME)) == 0) {
		bytes_written = wl_android_send_action_frame(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_REASSOC, strlen(CMD_REASSOC)) == 0) {
		bytes_written = wl_android_reassoc(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETSCANCHANNELTIME, strlen(CMD_GETSCANCHANNELTIME)) == 0) {
		bytes_written = wl_android_get_scan_channel_time(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSCANCHANNELTIME, strlen(CMD_SETSCANCHANNELTIME)) == 0) {
		bytes_written = wl_android_set_scan_channel_time(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETSCANHOMETIME, strlen(CMD_GETSCANHOMETIME)) == 0) {
		bytes_written = wl_android_get_scan_home_time(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSCANHOMETIME, strlen(CMD_SETSCANHOMETIME)) == 0) {
		bytes_written = wl_android_set_scan_home_time(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETSCANHOMEAWAYTIME, strlen(CMD_GETSCANHOMEAWAYTIME)) == 0) {
		bytes_written = wl_android_get_scan_home_away_time(net, command,
			priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSCANHOMEAWAYTIME, strlen(CMD_SETSCANHOMEAWAYTIME)) == 0) {
		bytes_written = wl_android_set_scan_home_away_time(net, command,
			priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETSCANNPROBES, strlen(CMD_GETSCANNPROBES)) == 0) {
		bytes_written = wl_android_get_scan_nprobes(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETSCANNPROBES, strlen(CMD_SETSCANNPROBES)) == 0) {
		bytes_written = wl_android_set_scan_nprobes(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETDFSSCANMODE, strlen(CMD_GETDFSSCANMODE)) == 0) {
		bytes_written = wl_android_get_scan_dfs_channel_mode(net, command,
			priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETDFSSCANMODE, strlen(CMD_SETDFSSCANMODE)) == 0) {
		bytes_written = wl_android_set_scan_dfs_channel_mode(net, command,
			priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETJOINPREFER, strlen(CMD_SETJOINPREFER)) == 0) {
		bytes_written = wl_android_set_join_prefer(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETWESMODE, strlen(CMD_GETWESMODE)) == 0) {
		bytes_written = wl_android_get_wes_mode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETWESMODE, strlen(CMD_SETWESMODE)) == 0) {
		bytes_written = wl_android_set_wes_mode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_GETOKCMODE, strlen(CMD_GETOKCMODE)) == 0) {
		bytes_written = wl_android_get_okc_mode(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SETOKCMODE, strlen(CMD_SETOKCMODE)) == 0) {
		bytes_written = wl_android_set_okc_mode(net, command, priv_cmd.total_len);
	}
#endif /* WES_SUPPORT */
#endif /* CUSTOMER_HW4 */

#ifdef PNO_SUPPORT
	else if (strnicmp(command, CMD_PNOSSIDCLR_SET, strlen(CMD_PNOSSIDCLR_SET)) == 0) {
		bytes_written = dhd_dev_pno_stop_for_ssid(net);
	}
#if !defined(WL_SCHED_SCAN) && defined(OEM_ANDROID) && defined(WL_WIRELESS_EXT)
	else if (strnicmp(command, CMD_PNOSETUP_SET, strlen(CMD_PNOSETUP_SET)) == 0) {
		bytes_written = wl_android_set_pno_setup(net, command, priv_cmd.total_len);
	}
#endif /* !WL_SCHED_SCAN && OEM_ANDROID && WL_WIRELESS_EXT */
	else if (strnicmp(command, CMD_PNOENABLE_SET, strlen(CMD_PNOENABLE_SET)) == 0) {
		int enable = *(command + strlen(CMD_PNOENABLE_SET) + 1) - '0';
		bytes_written = (enable)? 0 : dhd_dev_pno_stop_for_ssid(net);
	}
	else if (strnicmp(command, CMD_WLS_BATCHING, strlen(CMD_WLS_BATCHING)) == 0) {
		bytes_written = wls_parse_batching_cmd(net, command, priv_cmd.total_len);
	}
#endif /* PNO_SUPPORT */
	else if (strnicmp(command, CMD_P2P_DEV_ADDR, strlen(CMD_P2P_DEV_ADDR)) == 0) {
		bytes_written = wl_android_get_p2p_dev_addr(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_P2P_SET_NOA, strlen(CMD_P2P_SET_NOA)) == 0) {
		int skip = strlen(CMD_P2P_SET_NOA) + 1;
		bytes_written = wl_cfg80211_set_p2p_noa(net, command + skip,
			priv_cmd.total_len - skip);
	}
#ifdef WL_SDO
	else if (strnicmp(command, CMD_P2P_SD_OFFLOAD, strlen(CMD_P2P_SD_OFFLOAD)) == 0) {
		u8 *buf = command;
		u8 *cmd_id = NULL;
		int len;

		cmd_id = strsep((char **)&buf, " ");
		/* if buf == NULL, means no arg */
		if (buf == NULL)
			len = 0;
		else
			len = strlen(buf);

		bytes_written = wl_cfg80211_sd_offload(net, cmd_id, buf, len);
	}
#endif /* WL_SDO */
#ifdef WL_NAN
	else if (strnicmp(command, CMD_NAN, strlen(CMD_NAN)) == 0) {
		bytes_written = wl_cfg80211_nan_cmd_handler(net, command,
			priv_cmd.total_len);
	}
#endif /* WL_NAN */
#if !defined WL_ENABLE_P2P_IF
	else if (strnicmp(command, CMD_P2P_GET_NOA, strlen(CMD_P2P_GET_NOA)) == 0) {
		bytes_written = wl_cfg80211_get_p2p_noa(net, command, priv_cmd.total_len);
	}
#endif /* WL_ENABLE_P2P_IF */
	else if (strnicmp(command, CMD_P2P_SET_PS, strlen(CMD_P2P_SET_PS)) == 0) {
		int skip = strlen(CMD_P2P_SET_PS) + 1;
		bytes_written = wl_cfg80211_set_p2p_ps(net, command + skip,
			priv_cmd.total_len - skip);
	}
	else if (strnicmp(command, CMD_P2P_ECSA, strlen(CMD_P2P_ECSA)) == 0) {
		int skip = strlen(CMD_P2P_ECSA) + 1;
		bytes_written = wl_cfg80211_set_p2p_ecsa(net, command + skip,
			priv_cmd.total_len - skip);
	}
#ifdef WL_CFG80211
	else if (strnicmp(command, CMD_SET_AP_WPS_P2P_IE,
		strlen(CMD_SET_AP_WPS_P2P_IE)) == 0) {
		int skip = strlen(CMD_SET_AP_WPS_P2P_IE) + 3;
		bytes_written = wl_cfg80211_set_wps_p2p_ie(net, command + skip,
			priv_cmd.total_len - skip, *(command + skip - 2) - '0');
	}
#ifdef WLFBT
	else if (strnicmp(command, CMD_GET_FTKEY, strlen(CMD_GET_FTKEY)) == 0) {
		wl_cfg80211_get_fbt_key(command);
		bytes_written = FBT_KEYLEN;
	}
#endif /* WLFBT */
#endif /* WL_CFG80211 */
	else if (strnicmp(command, CMD_OKC_SET_PMK, strlen(CMD_OKC_SET_PMK)) == 0)
		bytes_written = wl_android_set_pmk(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_OKC_ENABLE, strlen(CMD_OKC_ENABLE)) == 0)
		bytes_written = wl_android_okc_enable(net, command, priv_cmd.total_len);
#ifdef BCMCCX
	else if (strnicmp(command, CMD_GETCCKM_RN, strlen(CMD_GETCCKM_RN)) == 0) {
		bytes_written = wl_android_get_cckm_rn(net, command);
	}
	else if (strnicmp(command, CMD_SETCCKM_KRK, strlen(CMD_SETCCKM_KRK)) == 0) {
		bytes_written = wl_android_set_cckm_krk(net, command);
	}
	else if (strnicmp(command, CMD_GET_ASSOC_RES_IES, strlen(CMD_GET_ASSOC_RES_IES)) == 0) {
		bytes_written = wl_android_get_assoc_res_ies(net, command);
	}
#endif /* BCMCCX */
#if defined(WL_SUPPORT_AUTO_CHANNEL)
	else if (strnicmp(command, CMD_GET_BEST_CHANNELS,
		strlen(CMD_GET_BEST_CHANNELS)) == 0) {
		bytes_written = wl_cfg80211_get_best_channels(net, command,
			priv_cmd.total_len);
	}
#endif /* WL_SUPPORT_AUTO_CHANNEL */
#if defined(WL_SUPPORT_AUTO_CHANNEL)
	else if (strnicmp(command, CMD_SET_HAPD_AUTO_CHANNEL,
		strlen(CMD_SET_HAPD_AUTO_CHANNEL)) == 0) {
		int skip = strlen(CMD_SET_HAPD_AUTO_CHANNEL) + 1;
		bytes_written = wl_android_set_auto_channel(net, (const char*)command+skip, command,
			priv_cmd.total_len);
	}
#endif /* WL_SUPPORT_AUTO_CHANNEL */
#ifdef CUSTOMER_HW4
#ifdef SUPPORT_AMPDU_MPDU_CMD
	/* CMD_AMPDU_MPDU */
	else if (strnicmp(command, CMD_AMPDU_MPDU, strlen(CMD_AMPDU_MPDU)) == 0) {
		int skip = strlen(CMD_AMPDU_MPDU) + 1;
		bytes_written = wl_android_set_ampdu_mpdu(net, (const char*)command+skip);
	}
#endif /* SUPPORT_AMPDU_MPDU_CMD */
#if defined(SUPPORT_HIDDEN_AP)
	else if (strnicmp(command, CMD_SET_HAPD_MAX_NUM_STA,
		strlen(CMD_SET_HAPD_MAX_NUM_STA)) == 0) {
		int skip = strlen(CMD_SET_HAPD_MAX_NUM_STA) + 3;
		wl_android_set_max_num_sta(net, (const char*)command+skip);
	}
	else if (strnicmp(command, CMD_SET_HAPD_SSID,
		strlen(CMD_SET_HAPD_SSID)) == 0) {
		int skip = strlen(CMD_SET_HAPD_SSID) + 3;
		wl_android_set_ssid(net, (const char*)command+skip);
	}
	else if (strnicmp(command, CMD_SET_HAPD_HIDE_SSID,
		strlen(CMD_SET_HAPD_HIDE_SSID)) == 0) {
		int skip = strlen(CMD_SET_HAPD_HIDE_SSID) + 3;
		wl_android_set_hide_ssid(net, (const char*)command+skip);
	}
#endif /* SUPPORT_HIDDEN_AP */
#ifdef SUPPORT_SOFTAP_SINGL_DISASSOC
	else if (strnicmp(command, CMD_HAPD_STA_DISASSOC,
		strlen(CMD_HAPD_STA_DISASSOC)) == 0) {
		int skip = strlen(CMD_HAPD_STA_DISASSOC) + 1;
		wl_android_sta_diassoc(net, (const char*)command+skip);
	}
#endif /* SUPPORT_SOFTAP_SINGL_DISASSOC */
#ifdef SUPPORT_SET_LPC
	else if (strnicmp(command, CMD_HAPD_LPC_ENABLED,
		strlen(CMD_HAPD_LPC_ENABLED)) == 0) {
		int skip = strlen(CMD_HAPD_LPC_ENABLED) + 3;
		wl_android_set_lpc(net, (const char*)command+skip);
	}
#endif /* SUPPORT_SET_LPC */
#ifdef SUPPORT_TRIGGER_HANG_EVENT
	else if (strnicmp(command, CMD_TEST_FORCE_HANG,
		strlen(CMD_TEST_FORCE_HANG)) == 0) {
		net_os_send_hang_message(net);
	}
#endif /* SUPPORT_TRIGGER_HANG_EVENT */
	else if (strnicmp(command, CMD_CHANGE_RL, strlen(CMD_CHANGE_RL)) == 0)
		bytes_written = wl_android_ch_res_rl(net, true);
	else if (strnicmp(command, CMD_RESTORE_RL, strlen(CMD_RESTORE_RL)) == 0)
		bytes_written = wl_android_ch_res_rl(net, false);
#ifdef SUPPORT_LTECX
	else if (strnicmp(command, CMD_LTECX_SET, strlen(CMD_LTECX_SET)) == 0) {
		int skip = strlen(CMD_LTECX_SET) + 1;
		bytes_written = wl_android_set_ltecx(net, (const char*)command+skip);
	}
#endif /* SUPPORT_LTECX */
	else if (strnicmp(command, CMD_SET_RMC_ENABLE, strlen(CMD_SET_RMC_ENABLE)) == 0) {
		int rmc_enable = *(command + strlen(CMD_SET_RMC_ENABLE) + 1) - '0';
		bytes_written = wl_android_rmc_enable(net, rmc_enable);
	}
	else if (strnicmp(command, CMD_SET_RMC_TXRATE, strlen(CMD_SET_RMC_TXRATE)) == 0) {
		int rmc_txrate;
		sscanf(command, "%*s %10d", &rmc_txrate);
		bytes_written = wldev_iovar_setint(net, "rmc_txrate", rmc_txrate * 2);
	}
	else if (strnicmp(command, CMD_SET_RMC_ACTPERIOD, strlen(CMD_SET_RMC_ACTPERIOD)) == 0) {
		int actperiod;
		sscanf(command, "%*s %10d", &actperiod);
		bytes_written = wldev_iovar_setint(net, "rmc_actf_time", actperiod);
	}
	else if (strnicmp(command, CMD_SET_RMC_IDLEPERIOD, strlen(CMD_SET_RMC_IDLEPERIOD)) == 0) {
		int acktimeout;
		sscanf(command, "%*s %10d", &acktimeout);
		acktimeout *= 1000;
		bytes_written = wldev_iovar_setint(net, "rmc_acktmo", acktimeout);
	}
	else if (strnicmp(command, CMD_SET_RMC_LEADER, strlen(CMD_SET_RMC_LEADER)) == 0) {
		int skip = strlen(CMD_SET_RMC_LEADER) + 1;
		bytes_written = wl_android_rmc_set_leader(net, (const char*)command+skip);
	}
	else if (strnicmp(command, CMD_SET_RMC_EVENT,
		strlen(CMD_SET_RMC_EVENT)) == 0)
		bytes_written = wl_android_set_rmc_event(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_GET_SCSCAN, strlen(CMD_GET_SCSCAN)) == 0) {
		bytes_written = wl_android_get_singlecore_scan(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_SET_SCSCAN, strlen(CMD_SET_SCSCAN)) == 0) {
		bytes_written = wl_android_set_singlecore_scan(net, command, priv_cmd.total_len);
	}
#ifdef TEST_TX_POWER_CONTROL
	else if (strnicmp(command, CMD_TEST_SET_TX_POWER,
		strlen(CMD_TEST_SET_TX_POWER)) == 0) {
		int skip = strlen(CMD_TEST_SET_TX_POWER) + 1;
		wl_android_set_tx_power(net, (const char*)command+skip);
	}
	else if (strnicmp(command, CMD_TEST_GET_TX_POWER,
		strlen(CMD_TEST_GET_TX_POWER)) == 0) {
		wl_android_get_tx_power(net, command, priv_cmd.total_len);
	}
#endif /* TEST_TX_POWER_CONTROL */
	else if (strnicmp(command, CMD_SARLIMIT_TX_CONTROL,
		strlen(CMD_SARLIMIT_TX_CONTROL)) == 0) {
		int skip = strlen(CMD_SARLIMIT_TX_CONTROL) + 1;
		wl_android_set_sarlimit_txctrl(net, (const char*)command+skip);
	}
#endif /* CUSTOMER_HW4 */
	else if (strnicmp(command, CMD_HAPD_MAC_FILTER, strlen(CMD_HAPD_MAC_FILTER)) == 0) {
		int skip = strlen(CMD_HAPD_MAC_FILTER) + 1;
		wl_android_set_mac_address_filter(net, (const char*)command+skip);
	}
	else if (strnicmp(command, CMD_SETROAMMODE, strlen(CMD_SETROAMMODE)) == 0)
		bytes_written = wl_android_set_roam_mode(net, command, priv_cmd.total_len);
#if defined(BCMFW_ROAM_ENABLE)
	else if (strnicmp(command, CMD_SET_ROAMPREF, strlen(CMD_SET_ROAMPREF)) == 0) {
		bytes_written = wl_android_set_roampref(net, command, priv_cmd.total_len);
	}
#endif /* BCMFW_ROAM_ENABLE */
	else if (strnicmp(command, CMD_MIRACAST, strlen(CMD_MIRACAST)) == 0)
		bytes_written = wl_android_set_miracast(net, command, priv_cmd.total_len);
#ifdef WL11ULB
	else if (strnicmp(command, CMD_ULB_MODE, strlen(CMD_ULB_MODE)) == 0)
		bytes_written = wl_android_set_ulb_mode(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_ULB_BW, strlen(CMD_ULB_BW)) == 0)
		bytes_written = wl_android_set_ulb_bw(net, command, priv_cmd.total_len);
#endif /* WL11ULB */
	else if (strnicmp(command, CMD_SETIBSSBEACONOUIDATA, strlen(CMD_SETIBSSBEACONOUIDATA)) == 0)
		bytes_written = wl_android_set_ibss_beacon_ouidata(net,
		command, priv_cmd.total_len);
#ifdef WLAIBSS
	else if (strnicmp(command, CMD_SETIBSSTXFAILEVENT,
		strlen(CMD_SETIBSSTXFAILEVENT)) == 0)
		bytes_written = wl_android_set_ibss_txfail_event(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_GET_IBSS_PEER_INFO_ALL,
		strlen(CMD_GET_IBSS_PEER_INFO_ALL)) == 0)
		bytes_written = wl_android_get_ibss_peer_info(net, command, priv_cmd.total_len,
			TRUE);
	else if (strnicmp(command, CMD_GET_IBSS_PEER_INFO,
		strlen(CMD_GET_IBSS_PEER_INFO)) == 0)
		bytes_written = wl_android_get_ibss_peer_info(net, command, priv_cmd.total_len,
			FALSE);
	else if (strnicmp(command, CMD_SETIBSSROUTETABLE,
		strlen(CMD_SETIBSSROUTETABLE)) == 0)
		bytes_written = wl_android_set_ibss_routetable(net, command,
			priv_cmd.total_len);
	else if (strnicmp(command, CMD_SETIBSSAMPDU, strlen(CMD_SETIBSSAMPDU)) == 0)
		bytes_written = wl_android_set_ibss_ampdu(net, command, priv_cmd.total_len);
	else if (strnicmp(command, CMD_SETIBSSANTENNAMODE, strlen(CMD_SETIBSSANTENNAMODE)) == 0)
		bytes_written = wl_android_set_ibss_antenna(net, command, priv_cmd.total_len);
#endif /* WLAIBSS */
	else if (strnicmp(command, CMD_KEEP_ALIVE, strlen(CMD_KEEP_ALIVE)) == 0) {
		int skip = strlen(CMD_KEEP_ALIVE) + 1;
		bytes_written = wl_keep_alive_set(net, command + skip, priv_cmd.total_len - skip);
	}
	else if (strnicmp(command, CMD_ROAM_OFFLOAD, strlen(CMD_ROAM_OFFLOAD)) == 0) {
		int enable = *(command + strlen(CMD_ROAM_OFFLOAD) + 1) - '0';
		bytes_written = wl_cfg80211_enable_roam_offload(net, enable);
	}
	else if (strnicmp(command, CMD_ROAM_OFFLOAD_APLIST, strlen(CMD_ROAM_OFFLOAD_APLIST)) == 0) {
		bytes_written = wl_android_set_roam_offload_bssid_list(net,
			command + strlen(CMD_ROAM_OFFLOAD_APLIST) + 1);
	}
#if defined(WL_VIRTUAL_APSTA)
	else if (strnicmp(command, CMD_INTERFACE_CREATE, strlen(CMD_INTERFACE_CREATE)) == 0) {
		char *name = (command + strlen(CMD_INTERFACE_CREATE) +1);
		WL_INFORM(("Creating %s interface\n", name));
		bytes_written = wl_cfg80211_interface_create(net, name);
	}
	else if (strnicmp(command, CMD_INTERFACE_DELETE, strlen(CMD_INTERFACE_DELETE)) == 0) {
		char *name = (command + strlen(CMD_INTERFACE_DELETE) +1);
		WL_INFORM(("Deleteing %s interface\n", name));
		bytes_written = wl_cfg80211_interface_delete(net, name);
	}
#endif /* defined (WL_VIRTUAL_APSTA) */
#ifdef P2PRESP_WFDIE_SRC
	else if (strnicmp(command, CMD_P2P_SET_WFDIE_RESP,
		strlen(CMD_P2P_SET_WFDIE_RESP)) == 0) {
		int mode = *(command + strlen(CMD_P2P_SET_WFDIE_RESP) + 1) - '0';
		bytes_written = wl_android_set_wfdie_resp(net, mode);
	} else if (strnicmp(command, CMD_P2P_GET_WFDIE_RESP,
		strlen(CMD_P2P_GET_WFDIE_RESP)) == 0) {
		bytes_written = wl_android_get_wfdie_resp(net, command, priv_cmd.total_len);
	}
#endif /* P2PRESP_WFDIE_SRC */
	else if (strnicmp(command, CMD_DFS_AP_MOVE, strlen(CMD_DFS_AP_MOVE)) == 0) {
		char *data = (command + strlen(CMD_DFS_AP_MOVE) +1);
		bytes_written = wl_cfg80211_dfs_ap_move(net, data, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_WBTEXT_ENABLE, strlen(CMD_WBTEXT_ENABLE)) == 0) {
		bytes_written = wl_android_wbtext(net, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_WBTEXT_PROFILE_CONFIG,
			strlen(CMD_WBTEXT_PROFILE_CONFIG)) == 0) {
		char *data = (command + strlen(CMD_WBTEXT_PROFILE_CONFIG) + 1);
		bytes_written = wl_cfg80211_wbtext_config(net, data, command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_WBTEXT_WEIGHT_CONFIG,
			strlen(CMD_WBTEXT_WEIGHT_CONFIG)) == 0) {
		char *data = (command + strlen(CMD_WBTEXT_WEIGHT_CONFIG) + 1);
		bytes_written = wl_cfg80211_wbtext_weight_config(net, data,
				command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_WBTEXT_TABLE_CONFIG,
			strlen(CMD_WBTEXT_TABLE_CONFIG)) == 0) {
		char *data = (command + strlen(CMD_WBTEXT_TABLE_CONFIG) + 1);
		bytes_written = wl_cfg80211_wbtext_table_config(net, data,
				command, priv_cmd.total_len);
	}
	else if (strnicmp(command, CMD_WBTEXT_DELTA_CONFIG,
			strlen(CMD_WBTEXT_DELTA_CONFIG)) == 0) {
		char *data = (command + strlen(CMD_WBTEXT_DELTA_CONFIG) + 1);
		bytes_written = wl_cfg80211_wbtext_delta_config(net, data,
				command, priv_cmd.total_len);
	}
#ifdef SET_RPS_CPUS
	else if (strnicmp(command, CMD_RPSMODE, strlen(CMD_RPSMODE)) == 0) {
		bytes_written = wl_android_set_rps_cpus(net, command, priv_cmd.total_len);
	}
#endif /* SET_RPS_CPUS */
#ifdef WLWFDS
	else if (strnicmp(command, CMD_ADD_WFDS_HASH, strlen(CMD_ADD_WFDS_HASH)) == 0) {
		bytes_written = wl_android_set_wfds_hash(net, command, priv_cmd.total_len, 1);
	}
	else if (strnicmp(command, CMD_DEL_WFDS_HASH, strlen(CMD_DEL_WFDS_HASH)) == 0) {
		bytes_written = wl_android_set_wfds_hash(net, command, priv_cmd.total_len, 0);
	}
#endif /* WLWFDS */
#ifdef BT_WIFI_HANDOVER
	else if (strnicmp(command, CMD_TBOW_TEARDOWN, strlen(CMD_TBOW_TEARDOWN)) == 0) {
	    bytes_written = wl_tbow_teardown(net, command, priv_cmd.total_len);
	}
#endif /* BT_WIFI_HANDOVER */
#ifdef CONNECTION_STATISTICS
	else if (strnicmp(command, CMD_GET_CONNECTION_STATS,
		strlen(CMD_GET_CONNECTION_STATS)) == 0) {
		bytes_written = wl_android_get_connection_stats(net, command,
			priv_cmd.total_len);
	}
#endif
	else {
		DHD_ERROR(("Unknown PRIVATE command %s - ignored\n", command));
		snprintf(command, 5, "FAIL");
		bytes_written = strlen("FAIL");
	}

	return bytes_written;
}

int wl_android_init(void)
{
	int ret = 0;

#ifdef ENABLE_INSMOD_NO_FW_LOAD
	dhd_download_fw_on_driverload = FALSE;
#endif /* ENABLE_INSMOD_NO_FW_LOAD */
	if (!iface_name[0]) {
		memset(iface_name, 0, IFNAMSIZ);
		bcm_strncpy_s(iface_name, IFNAMSIZ, "wlan", IFNAMSIZ);
	}

#ifdef WL_GENL
	wl_genl_init();
#endif
	wl_netlink_init();

	return ret;
}

int wl_android_exit(void)
{
	//DHD_ERROR(("%s in\n", __FUNCTION__));
	//printk("%s: \n", __FUNCTION__);
	int ret = 0;
	struct io_cfg *cur, *q;

#ifdef WL_GENL
	wl_genl_deinit();
#endif /* WL_GENL */
	wl_netlink_deinit();

	list_for_each_entry_safe(cur, q, &miracast_resume_list, list) {
		list_del(&cur->list);
		kfree(cur);
	}
	//DHD_ERROR(("%s out\n", __FUNCTION__));
	return ret;
}

void wl_android_post_init(void)
{

#ifdef ENABLE_4335BT_WAR
	bcm_bt_unlock(lock_cookie_wifi);
	printk("%s: btlock released\n", __FUNCTION__);
#endif /* ENABLE_4335BT_WAR */

	if (!dhd_download_fw_on_driverload)
		g_wifi_on = FALSE;
}

#ifdef WL_GENL
/* Generic Netlink Initializaiton */
static int wl_genl_init(void)
{
	int ret;

	WL_DBG(("GEN Netlink Init\n\n"));

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	/* register new family */
	ret = genl_register_family(&wl_genl_family);
	if (ret != 0)
		goto failure;

	/* register functions (commands) of the new family */
	ret = genl_register_ops(&wl_genl_family, &wl_genl_ops);
	if (ret != 0) {
		WL_ERR(("register ops failed: %i\n", ret));
		genl_unregister_family(&wl_genl_family);
		goto failure;
	}

	ret = genl_register_mc_group(&wl_genl_family, &wl_genl_mcast);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	ret = genl_register_family(&wl_genl_family);
	if (ret != 0)
		goto failure;
#else
	ret = genl_register_family_with_ops_groups(&wl_genl_family, wl_genl_ops, wl_genl_mcast);
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0) */
	if (ret != 0) {
		WL_ERR(("register mc_group failed: %i\n", ret));
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
		genl_unregister_ops(&wl_genl_family, &wl_genl_ops);
#endif
		genl_unregister_family(&wl_genl_family);
		goto failure;
	}

	return 0;

failure:
	WL_ERR(("Registering Netlink failed!!\n"));
	return -1;
}

/* Generic netlink deinit */
static int wl_genl_deinit(void)
{

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	if (genl_unregister_ops(&wl_genl_family, &wl_genl_ops) < 0)
		WL_ERR(("Unregister wl_genl_ops failed\n"));
#endif
	if (genl_unregister_family(&wl_genl_family) < 0)
		WL_ERR(("Unregister wl_genl_ops failed\n"));

	return 0;
}

s32 wl_event_to_bcm_event(u16 event_type)
{
	u16 event = -1;

	switch (event_type) {
		case WLC_E_SERVICE_FOUND:
			event = BCM_E_SVC_FOUND;
			break;
		case WLC_E_P2PO_ADD_DEVICE:
			event = BCM_E_DEV_FOUND;
			break;
		case WLC_E_P2PO_DEL_DEVICE:
			event = BCM_E_DEV_LOST;
			break;
	/* Above events are supported from BCM Supp ver 47 Onwards */
#ifdef BT_WIFI_HANDOVER
		case WLC_E_BT_WIFI_HANDOVER_REQ:
			event = BCM_E_DEV_BT_WIFI_HO_REQ;
			break;
#endif /* BT_WIFI_HANDOVER */

		default:
			WL_ERR(("Event not supported\n"));
	}

	return event;
}

s32
wl_genl_send_msg(
	struct net_device *ndev,
	u32 event_type,
	u8 *buf,
	u16 len,
	u8 *subhdr,
	u16 subhdr_len)
{
	int ret = 0;
	struct sk_buff *skb;
	void *msg;
	u32 attr_type = 0;
	bcm_event_hdr_t *hdr = NULL;
	int mcast = 1; /* By default sent as mutlicast type */
	int pid = 0;
	u8 *ptr = NULL, *p = NULL;
	u32 tot_len = sizeof(bcm_event_hdr_t) + subhdr_len + len;
	u16 kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;


	WL_DBG(("Enter \n"));

	/* Decide between STRING event and Data event */
	if (event_type == 0)
		attr_type = BCM_GENL_ATTR_STRING;
	else
		attr_type = BCM_GENL_ATTR_MSG;

	skb = genlmsg_new(NLMSG_GOODSIZE, kflags);
	if (skb == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	msg = genlmsg_put(skb, 0, 0, &wl_genl_family, 0, BCM_GENL_CMD_MSG);
	if (msg == NULL) {
		ret = -ENOMEM;
		goto out;
	}


	if (attr_type == BCM_GENL_ATTR_STRING) {
		/* Add a BCM_GENL_MSG attribute. Since it is specified as a string.
		 * make sure it is null terminated
		 */
		if (subhdr || subhdr_len) {
			WL_ERR(("No sub hdr support for the ATTR STRING type \n"));
			ret =  -EINVAL;
			goto out;
		}

		ret = nla_put_string(skb, BCM_GENL_ATTR_STRING, buf);
		if (ret != 0) {
			WL_ERR(("nla_put_string failed\n"));
			goto out;
		}
	} else {
		/* ATTR_MSG */

		/* Create a single buffer for all */
		p = ptr = kzalloc(tot_len, kflags);
		if (!ptr) {
			ret = -ENOMEM;
			WL_ERR(("ENOMEM!!\n"));
			goto out;
		}

		/* Include the bcm event header */
		hdr = (bcm_event_hdr_t *)ptr;
		hdr->event_type = wl_event_to_bcm_event(event_type);
		hdr->len = len + subhdr_len;
		ptr += sizeof(bcm_event_hdr_t);

		/* Copy subhdr (if any) */
		if (subhdr && subhdr_len) {
			memcpy(ptr, subhdr, subhdr_len);
			ptr += subhdr_len;
		}

		/* Copy the data */
		if (buf && len) {
			memcpy(ptr, buf, len);
		}

		ret = nla_put(skb, BCM_GENL_ATTR_MSG, tot_len, p);
		if (ret != 0) {
			WL_ERR(("nla_put_string failed\n"));
			goto out;
		}
	}

	if (mcast) {
		int err = 0;
		/* finalize the message */
		genlmsg_end(skb, msg);
		/* NETLINK_CB(skb).dst_group = 1; */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
		if ((err = genlmsg_multicast(skb, 0, wl_genl_mcast.id, GFP_ATOMIC)) < 0)
#else
		if ((err = genlmsg_multicast(&wl_genl_family, skb, 0, 0, GFP_ATOMIC)) < 0)
#endif
			WL_ERR(("genlmsg_multicast for attr(%d) failed. Error:%d \n",
				attr_type, err));
		else
			WL_DBG(("Multicast msg sent successfully. attr_type:%d len:%d \n",
				attr_type, tot_len));
	} else {
		NETLINK_CB(skb).dst_group = 0; /* Not in multicast group */

		/* finalize the message */
		genlmsg_end(skb, msg);

		/* send the message back */
		if (genlmsg_unicast(&init_net, skb, pid) < 0)
			WL_ERR(("genlmsg_unicast failed\n"));
	}

out:
	if (p)
		kfree(p);
	if (ret)
		nlmsg_free(skb);

	return ret;
}

static s32
wl_genl_handle_msg(
	struct sk_buff *skb,
	struct genl_info *info)
{
	struct nlattr *na;
	u8 *data = NULL;

	WL_DBG(("Enter \n"));

	if (info == NULL) {
		return -EINVAL;
	}

	na = info->attrs[BCM_GENL_ATTR_MSG];
	if (!na) {
		WL_ERR(("nlattribute NULL\n"));
		return -EINVAL;
	}

	data = (char *)nla_data(na);
	if (!data) {
		WL_ERR(("Invalid data\n"));
		return -EINVAL;
	} else {
		/* Handle the data */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)) || defined(WL_COMPAT_WIRELESS)
		WL_DBG(("%s: Data received from pid (%d) \n", __func__,
			info->snd_pid));
#else
		WL_DBG(("%s: Data received from pid (%d) \n", __func__,
			info->snd_portid));
#endif /* (LINUX_VERSION < VERSION(3, 7, 0) || WL_COMPAT_WIRELESS */
	}

	return 0;
}
#endif /* WL_GENL */
