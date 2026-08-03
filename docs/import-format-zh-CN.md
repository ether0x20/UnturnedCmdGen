# 数据表格 JSON 导入格式标准

所有数据表（物品、载具、动物、效果、任务、成就、技能组、玩家）使用**完全相同的 JSON 结构**，可通过 `File → Import JSON...` 导入。

---

## 一、总体格式

文件顶层必须是一个 **JSON 数组（array）**，数组每个元素是一个 **对象（object）**，代表一条记录。

```json
[
  { "id": "116", "name": "Military Knife", "nameZh": "军用刀", "note": "melee" },
  { "id": "363", "name": "Maple Rifle", "nameZh": "枫木步枪" }
]
```

## 二、字段定义

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `id` | string | 否* | 主键。物品/载具等填游戏内 ID；玩家填 SteamID。作为字符串存储（数字也写字符串） |
| `name` | string | 否* | 英文显示名称。**必填之一** |
| `nameZh` | string | 否 | 中文标注，用于下拉框双语显示 `[id] name / 中文` 及中文搜索 |
| `note` | string | 否 | 附加说明，仅作显示用途 |

> *规则：`id`、`name`、`nameZh` 不能同时为空，否则该条记录会被跳过。

### 主键字段的兼容别名

| 表类型 | 推荐字段 | 兼容字段 |
|--------|----------|----------|
| 物品/载具/动物/效果/任务/成就/技能组 | `id` | `key` |
| 玩家 | `steamId` | `id` |

---

## 三、各表示例

### 物品表（items）
```json
[
  { "id": "116", "name": "Military Knife", "nameZh": "军用刀", "note": "melee" },
  { "id": "363", "name": "Maple Rifle", "nameZh": "枫木步枪" }
]
```

### 载具表（vehicles）
```json
[
  { "id": "1", "name": "Offroader", "nameZh": "越野车" },
  { "id": "17", "name": "Helicopter", "nameZh": "直升机", "note": "rare spawn" }
]
```

### 玩家表（players）
```json
[
  { "steamId": "76561198000000001", "name": "Ethan", "note": "服主" },
  { "steamId": "76561198000000002", "name": "Player2" }
]
```

> 玩家表通常不填 `nameZh`（玩家名无需翻译），但字段同样支持。

### 动物表（animals）
```json
[
  { "id": "5", "name": "Wolf", "nameZh": "狼" },
  { "id": "6", "name": "Bear", "nameZh": "熊" }
]
```

### 枚举类表（效果 / 任务 / 成就 / 技能组）
```json
[
  { "id": "1", "name": "Find Aid", "nameZh": "寻找援助" },
  { "id": "255", "name": "All Skillsets", "nameZh": "全部技能组" }
]
```

---

## 四、导入行为

通过 `File → Import JSON...` 打开导入对话框，可点击 **"? Import Tutorial"** 查看内置的格式实例。

导入时程序会询问 **合并或替换**：

- **合并（Merge）**：追加到现有表；`id` 重复的记录被跳过。
- **替换（Replace）**：现有内容全部清除，仅保留导入文件内容。

**导入失败的判定**：文件不是合法 JSON、顶层不是数组、或所有记录均被跳过（`id`/`name`/`nameZh` 全空）时，导入失败，并提示可查看导入教程。

## 五、界内表格管理

导入后的数据（包括程序自带的种子数据）可在 **`Data` 菜单**中按表管理：

- `Manage Items...` / `Manage Vehicles...` / `Manage Animals...` / `Manage Effects...` 等
- 对话框内可**增删改查** `ID / 名称 / 中文名 / 备注` 四列
- 点击 **Save** 会写回该表来源的 JSON 文件：
  - 玩家表 → 用户数据目录 `players.json`
  - 其它表 → 程序 `assets/` 目录下的对应 JSON（如 `assets/items.json`）

> 提示：若 `assets/` 所在目录只读（例如安装到系统目录），编辑后保存会失败，请先通过 `File → Export JSON...` 导出，或将数据文件放到可写位置。

## 六、生成指令时用到哪个字段

| 字段 | 用途 |
|------|------|
| `name` | 查找表下拉框显示 `[id] name / 中文`；选中后**作为参数值**填入指令（如 `/give Player Maplestrike`） |
| `nameZh` | 下拉框中文显示 + 中文搜索；仅作展示，不进入指令 |
| `id` | 下拉框中显示；玩家表手动输入 SteamID 时以原文本填入 |

## 七、快速校验

可用任意 JSON 工具校验文件后再导入：

```bash
python3 -c "import json,sys; json.load(open(sys.argv[1])); print('valid')" my_table.json
```
