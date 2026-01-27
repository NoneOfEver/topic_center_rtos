# Git Commit Message Template

> 每次 commit 可以从这里复制，保持格式统一  
> 支持 type + emoji 风格，便于可读性和生成 changelog

---

## 1️⃣ Commit 格式

<emoji> <type>(<scope>): <subject>

<body> <footer> ```
1.1 各部分说明

emoji：可选，增加可读性

type：提交类型

scope：修改模块或文件名，可选

subject：简短描述（50 字以内，不加句号）

body：详细描述修改原因和内容（每行 72 字以内，可选）

footer：关联 issue 或说明 breaking change（可选）

2️⃣ Type + Emoji 对照表
Emoji	Type	说明
✨	feat	新功能
🐛	fix	修复 bug
📝	docs	文档更新
💄	style	代码格式调整（不影响功能）
♻️	refactor	代码重构（不新增功能/不修复 bug）
⚡	perf	性能优化
✅	test	新增/修改测试
🔧	chore	构建/依赖/工具更新
🔀	merge	分支合并
3️⃣ 示例
✨ feat(auth): add login with Google

📝 docs(readme): update installation instructions

🐛 fix(cart): correct total price calculation

💄 style(header): adjust spacing and indentation

♻️ refactor(user): simplify login flow

⚡ perf(cache): optimize query performance

✅ test(auth): add unit tests for login

🔧 chore(ci): update GitHub Actions workflow

🔀 merge: merge branch 'feature/cart' into 'main'

4️⃣ 使用建议

每次 commit 前从模板中复制一条格式化的 commit message

subject 尽量用动词开头，如 add, fix, update

body 说明“为什么改”和“怎么改”，不要重复代码内容

footer 可写 Closes #issue_number 来关联问题