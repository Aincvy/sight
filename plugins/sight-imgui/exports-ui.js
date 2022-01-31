print(typeof sight.entity.addOperation);

sight.entity.addOperation('imgui', 'desc', function(entity){
    print(123);
    print(entity.name);
    return entity.templateAddress;
});

