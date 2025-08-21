#!/bin/bash

# 通用commit脚本
# 功能：创建分支 -> 提交变更 -> 推送 -> 创建PR -> 合并 -> 清理

# 检查是否有提交信息
if [ $# -eq 0 ]; then
    echo "用法: ./commit.sh \"提交信息\" [分支名称]"
    echo "示例: ./commit.sh \"fix: 修复mdns服务问题\""
    echo "示例: ./commit.sh \"feat: 添加新功能\" \"feature/new-feature\""
    exit 1
fi

# 获取提交信息
COMMIT_MESSAGE="$1"

# 生成分支名称（如果没有提供）
if [ $# -eq 2 ]; then
    BRANCH_NAME="$2"
else
    # 从提交信息中提取类型和描述来生成分支名
    COMMIT_TYPE=$(echo "$COMMIT_MESSAGE" | cut -d: -f1)
    COMMIT_DESC=$(echo "$COMMIT_MESSAGE" | cut -d: -f2 | xargs) # xargs用于去除前导空格
    
    # 将描述转换为小写并替换空格为连字符
    DESC_FORMATTED=$(echo "$COMMIT_DESC" | tr '[:upper:]' '[:lower:]' | tr ' ' '-')
    
    # 生成分支名
    BRANCH_NAME="$COMMIT_TYPE/$DESC_FORMATTED"
fi

echo "开始执行提交流程..."
echo "提交信息: $COMMIT_MESSAGE"
echo "分支名称: $BRANCH_NAME"
echo ""

# 1. 创建新的分支
echo "1. 创建新分支: $BRANCH_NAME"
git checkout -b "$BRANCH_NAME"
if [ $? -ne 0 ]; then
    echo "创建分支失败"
    exit 1
fi

# 2. 提交所有未提交的变更到分支
echo "2. 提交所有变更"
git add .
git commit -m "$COMMIT_MESSAGE"
if [ $? -ne 0 ]; then
    echo "提交失败"
    exit 1
fi

# 3. 把代码推送到github
echo "3. 推送代码到GitHub"
git push origin "$BRANCH_NAME"
if [ $? -ne 0 ]; then
    echo "推送失败"
    exit 1
fi

# 4. 用gh创建pr
echo "4. 创建Pull Request"
gh pr create --title "$COMMIT_MESSAGE" --body "自动创建的PR，包含以下变更：\n\n$COMMIT_MESSAGE" --head "$BRANCH_NAME" --base main
if [ $? -ne 0 ]; then
    echo "创建PR失败"
    exit 1
fi

# 5. 合并pr，删除分支
echo "5. 合并PR并删除分支"
gh pr merge --auto --merge --delete-branch
if [ $? -ne 0 ]; then
    echo "合并PR失败"
    exit 1
fi

# 6. 返回main分支，并从远程更新
echo "6. 切换回main分支并更新"
git checkout main
git pull origin main
if [ $? -ne 0 ]; then
    echo "切换分支或更新失败"
    exit 1
fi

echo ""
echo "✅ 提交流程完成！"
echo "   - 分支 $BRANCH_NAME 已创建并推送"
echo "   - PR已创建并自动合并"
echo "   - 分支已删除"
echo "   - 已回到main分支并更新"