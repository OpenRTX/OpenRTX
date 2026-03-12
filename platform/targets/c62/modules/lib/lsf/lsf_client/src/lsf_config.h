#ifndef __LSF_CONFIG_H__
#define __LSF_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

// 核间属性 group ID 最大值. 必须AP/CP对等
#define LSF_PROPERTY_GROUP_COUNT    (16)

// set/get同步用的msgQ存放在thread local storage, 约定其位置INDEX.
#define LSF_PROPERTY_TLS_INDEX      (0)

#ifdef __cplusplus
}
#endif

#endif /* __LSF_CONFIG_H__ */
