#ifndef __LSF_H__
#define __LSF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lsf_property_common.h"
#include "variant.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 错误码------------------------------------------------
#define LSF_SUCCESS                       (0)
#define LSF_ERROR                         (-1)

#define LSF_ERROR_PARAMETER               (-2)
#define LSF_ERROR_TIMEOUT                 (-3)

// API------------------------------------------------
/**
  \brief       lsf模块初始化
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_init(void);

/**
  \brief       连接ap/cp, 仅核间服务的通路建立, 可供查询 核间属性 对象
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_connect(void);

// TODO: 查询可用属性? 场景? 连带需要动态建立 核间属性
// int lsf_query(lsf* handle, int property_id);

/*
    main处发布一张总清单以启用 属性服务, 或, 各service处attach其局部清单?
        对ap来说, 各service-client(AP)只关心其使用到的 service核间属性 清单.
        这样, 局部清单模块性更好.
        lsf_publish(核间属性 局部清单);
        对外declare一组 核间属性; 对称的, undeclare;
*/
int lsf_export_properties(int group_id, LsfPropertyConfig *config_array, int config_count);

/*
核间属性 激活, 到此, 才真正建立 核间属性对象, 对端才可获取 本对象 的proxy端
此前, 对端仅可查询到 未就绪, 无法建立proxy对象进行通讯
核间属性 配置表仅表示座位表, 其中没有实际对象, register后才有实际对象通讯
    如果对象未创建, 则创建核间对象 再设置它; 如果已经创建, 则直接设置它
*/
int lsf_register(int property_group_id, int property_id,
                 LsfProperty_OnSetFunc onset_func, LsfProperty_OnGetFunc onget_func,
                 LSF_PROPERTY_CONTEXT_TYPE context_type, void *context_param);

/**
  \brief       设置 核间属性, 同步阻塞
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_set(int group_id, int property_id, LsfVariant *var);

/**
  \brief       读取 核间属性, 同步阻塞
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_get(int group_id, int property_id, LsfVariant **out_var);

// typedef enum {
//     // 运行上下文在task中
//     LSF_PROPERTY_CONTEXT_TASK,

//     // 没有指定运行上下文
//     LSF_PROPERTY_CONTEXT_NONE,
// } LSF_PROPERTY_CONTEXT_TYPE;

// // onSet方法
// typedef int (*LsfPropertyOnSetFunc)(int property_id, LsfVariant *new_var);

// // onGet方法
// typedef int (*LsfPropertyOnGetFunc)(int property_id, LsfVariant *new_var);

/**
  \brief       发出通知, 异步
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_notify(int group_id, int property_id, LsfVariant *var);

/**
  \brief       lsf消息处理, 需要嵌入在支持lsf的消息循环中
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_defaultMsgProc(LsfMsg *msg);

/**
  \brief       断开连接ap/cp
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_disconnect();

/**
  \brief       lsf模块去初始化
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int lsf_uninit();

#ifdef __cplusplus
}
#endif

#endif /* __LSF_H__ */
