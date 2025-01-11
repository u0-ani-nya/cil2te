import re
import sys
import os

def remove_version_suffix(name):
    """移除版本号后缀"""
    return re.sub(r'_\d+(_\d+)*$', '', name)

def convert_cil_to_te(cil_content):
    te_content = []
    type_attributes = {}
    types = set()
    expand_type_attributes = set()

    # 解析所有 typeattributeset 语句
    typeattributeset_pattern = re.compile(r'\(typeattributeset (\S+) \(([^)]+)\)\)')
    for match in typeattributeset_pattern.finditer(cil_content):
        attribute_set, elements = match.groups()
        attribute_set = remove_version_suffix(attribute_set)
        elements_list = elements.split()
        for element in elements_list:
            element = remove_version_suffix(element)
            if element not in type_attributes:
                type_attributes[element] = []
            type_attributes[element].append(attribute_set)

    # 解析所有 type 语句
    type_pattern = re.compile(r'\(type (\S+)\)')
    for match in type_pattern.finditer(cil_content):
        type_name = remove_version_suffix(match.groups()[0])
        types.add(type_name)

    # 解析 expandtypeattribute 语句
    expandtypeattribute_pattern = re.compile(r'\(expandtypeattribute \((\S+)\) (\S+)\)')
    for match in expandtypeattribute_pattern.finditer(cil_content):
        type_attr, expand = match.groups()
        type_attr = remove_version_suffix(type_attr)
        if expand == "true":
            expand_type_attributes.add(type_attr)

    # 处理 typetransition 语句
    typetransition_pattern = re.compile(r'\(typetransition (\S+) (\S+) (\S+) (\S+)\)')
    for match in typetransition_pattern.finditer(cil_content):
        source_type, target_type, class_type, new_type = match.groups()
        source_type = remove_version_suffix(source_type)
        target_type = remove_version_suffix(target_type)
        class_type = remove_version_suffix(class_type)
        new_type = remove_version_suffix(new_type)
        te_content.append(f"type_transition {source_type} {target_type}:{class_type} {new_type};")

    # 处理 allow 语句
    allow_pattern = re.compile(r'\(allow (\S+) (\S+) \((\S+) \(([^)]+)\)\)\)')
    for match in allow_pattern.finditer(cil_content):
        source_type, target_type, class_type, permissions = match.groups()
        source_type = remove_version_suffix(source_type)
        target_type = remove_version_suffix(target_type)
        class_type = remove_version_suffix(class_type)
        permissions_list = permissions.split()
        permissions_str = " ".join(permissions_list)
        te_content.append(f"allow {source_type} {target_type}:{class_type} {{ {permissions_str} }};")

    # 处理 attribute 语句
    attribute_pattern = re.compile(r'\(attribute (\S+)\)')
    for match in attribute_pattern.finditer(cil_content):
        attribute = match.groups()[0]
        attribute = remove_version_suffix(attribute)
        te_content.append(f"attribute {attribute};")

    # 处理 typeattribute 语句
    typeattribute_pattern = re.compile(r'\(typeattribute (\S+)\)')
    for match in typeattribute_pattern.finditer(cil_content):
        type_attr = match.groups()[0]
        type_attr = remove_version_suffix(type_attr)
        te_content.append(f"attribute {type_attr};")

    # 处理 role 语句
    role_pattern = re.compile(r'\(role (\S+)\)')
    for match in role_pattern.finditer(cil_content):
        role = match.groups()[0]
        role = remove_version_suffix(role)
        if role in type_attributes:
            for attr in type_attributes[role]:
                te_content.append(f"type {role}, {attr};")

    # 处理 roletype 语句
    roletype_pattern = re.compile(r'\(roletype (\S+) (\S+)\)')
    for match in roletype_pattern.finditer(cil_content):
        role, type_name = match.groups()
        role = remove_version_suffix(role)
        type_name = remove_version_suffix(type_name)
        if type_name in type_attributes:
            te_content.append(f"type {type_name}, {', '.join(type_attributes[type_name])};")
        else:
            te_content.append(f"type {type_name};")

    # 处理 roleattribute 语句
    roleattribute_pattern = re.compile(r'\(roleattribute (\S+)\)')
    for match in roleattribute_pattern.finditer(cil_content):
        roleattribute = match.groups()[0]
        roleattribute = remove_version_suffix(roleattribute)
        if roleattribute in type_attributes:
            for attr in type_attributes[roleattribute]:
                te_content.append(f"attribute {roleattribute}, {attr};")

    # 处理其他常见的CIL规则
    other_patterns = {
        'genfscon': re.compile(r'\(genfscon (\S+) (\S+) \(([^)]+)\)\)'),
        'classcommon': re.compile(r'\(classcommon (\S+) (\S+)\)'),
        'class': re.compile(r'\(class (\S+) \(([^)]+)\)\)'),
        'mlsconstrain': re.compile(r'\(mlsconstrain \((\S+) \(([^)]+)\)\) \(([^)]+)\)\)'),
        'fsuse': re.compile(r'\(fsuse (\S+) (\S+) \(([^)]+)\)\)'),
        'sidcontext': re.compile(r'\(sidcontext (\S+) \(([^)]+)\)\)'),
        'sid': re.compile(r'\(sid (\S+)\)'),
        'handleunknown': re.compile(r'\(handleunknown (\S+)\)'),
        'mls': re.compile(r'\(mls (\S+)\)'),
        'policycap': re.compile(r'\(policycap (\S+)\)'),
    }

    for key, pattern in other_patterns.items():
        for match in pattern.finditer(cil_content):
            groups = match.groups()
            groups = [remove_version_suffix(g) for g in groups]
            if key == 'classcommon':
                te_content.append(f"class {groups[0]} common {groups[1]};")
            elif key == 'class':
                permissions = groups[1].split()
                permissions_str = " ".join(permissions)
                te_content.append(f"class {groups[0]} {{ {permissions_str} }};")
            elif key == 'mlsconstrain':
                constraints = groups[2].split()
                constraints_str = " ".join(constraints)
                te_content.append(f"mlsconstrain {groups[0]} {{ {groups[1]} }} {constraints_str};")
            else:
                te_content.append(f"{key} {' '.join(groups)};")

    # 生成 type 语句
    for type_name in types:
        if type_name in type_attributes:
            te_content.append(f"type {type_name}, {', '.join(type_attributes[type_name])};")
        else:
            te_content.append(f"type {type_name};")

    # 生成 expandtypeattribute 语句
    for type_attr in expand_type_attributes:
        te_content.append(f"attribute {type_attr};")

    return "\n".join(te_content)

def main():
    # 检查命令行参数
    if len(sys.argv) != 2:
        print("usage: python ./trans.py <file>")
        return

    cil_file_path = sys.argv[1]

    # 检查文件扩展名
    if not cil_file_path.endswith('.cil'):
        print("Error: Input file must have a .cil extension.")
        return

    # 读取CIL文件内容
    try:
        with open(cil_file_path, "r") as cil_file:
            cil_content = cil_file.read()
    except FileNotFoundError:
        print(f"Error: File '{cil_file_path}' not found.")
        return

    # 转换为TE文件格式
    te_content = convert_cil_to_te(cil_content)

    # 生成输出文件名
    output_file_path = os.path.splitext(cil_file_path)[0] + ".te"

    # 写入TE文件
    with open(output_file_path, "w") as te_file:
        te_file.write(te_content)

    print(f"Conversion complete. TE content has been written to '{output_file_path}'.")

if __name__ == "__main__":
    main()
