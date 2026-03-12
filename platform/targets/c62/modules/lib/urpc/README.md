CSK6 URPC 统一仓库
==========

改仓库统一管理 CSK6 URPC 的 AP 和 CP 代码。

## 引入方式

在上层 `CMakeLists.txt` 里引入 `targets` 目录下对应的子目录即可：

#### CP

```cmake
add_subdirectory(urpc/targets/cp_xos)

# 在 app 里
target_link_libraries(app PRIVATE urpc)
```

#### AP (FreeRTOS)

```cmake
add_subdirectory(urpc/targets/ap_freertos)

# 在 app 里
target_link_libraries(app PRIVATE urpc)
```

#### AP (Zephyr)

可直接作为 External Module 通过 `west.yml` 引入：

```yaml
manifest:
  remotes:
    - name: lsf-dev
      url-base: https://cloud.listenai.com/listenai/lsf-dev
  projects:
    - name: freertos_shims
      remote: lsf-dev
      revision: master
      path: modules/lib/freertos_shims
    - name: urpc
      remote: lsf-dev
      revision: master
      path: modules/lib/urpc
```

> **注意:** 本仓目前需要依赖 [freertos_shims](https://cloud.listenai.com/listenai/lsf-dev/freertos_shims) 模块

对应 `CMakeLists.txt` 改动：

```cmake
# 在 app 里
target_link_libraries(app PRIVATE urpc)

# 或者在 library 里
zephyr_library_link_libraries(urpc)
```
