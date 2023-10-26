## Built-in types

### Built-in cpp types 

Here is the types registered by cpp.

```cpp
        ImU32 color = IM_COL32(0, 99, 160, 255);
        addTypeInfo({ .name = "void", .intValue = IntTypeVoid }, { IM_COL32(255,255,255,255), IconType::Circle });
        addTypeInfo({ .name = "Process", .intValue = IntTypeProcess }, { color, IconType::Flow });
        addTypeInfo({ .name = "int", .intValue = IntTypeInt}, { color, IconType::Circle });
        addTypeInfo({ .name = "float", .intValue = IntTypeFloat }, { color, IconType::Circle });
        addTypeInfo({ .name = "double", .intValue = IntTypeDouble }, { color, IconType::Circle });
        addTypeInfo({ .name = "long", .intValue = IntTypeLong }, { color, IconType::Circle });
        addTypeInfo({ .name = "bool", .intValue = IntTypeBool }, { color, IconType::Circle });
        addTypeInfo({ .name = "char", .intValue = IntTypeChar }, { color, IconType::Circle });
        addTypeInfo({ .name = "Color", .intValue = IntTypeColor }, { color, IconType::Circle });
        addTypeInfo({ .name = "Vector3", .intValue = IntTypeVector3 }, { color, IconType::Circle });
        addTypeInfo({ .name = "Vector4", .intValue = IntTypeVector4 }, { color, IconType::Circle });
        addTypeInfo({ .name = "String", .intValue = IntTypeString }, { color, IconType::Circle });
        addTypeInfo({ .name = "LargeString", .intValue = IntTypeLargeString }, { color, IconType::Circle });
        addTypeInfo({ .name = "Object", .intValue = IntTypeObject }, { color, IconType::Circle });
        addTypeInfo({ .name = "button", .intValue = IntTypeButton }, { color, IconType::Circle });

        // type alias
        typeMap["Number"] = IntTypeFloat;
        typeMap["process"] = IntTypeProcess;
        typeMap["string"] = IntTypeString;
        typeMap["object"] = IntTypeObject;
        typeMap["number"] = IntTypeFloat;
        typeMap["Flow"] = IntTypeProcess;
        typeMap["boolean"] = IntTypeBool;
```

The `.name` part is valid.


### sight-base plugin's types





## Custom Types

 `function addType(type, options)` can add a type.

