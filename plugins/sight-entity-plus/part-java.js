function javaEntityLombok(entity) {
    let str = 
`
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.NoArgsConstructor;

/**
 * Generated by sight(sight-entity-plus plugin) on <%=Date()%>
 */
@Data
@AllArgsConstructor
@NoArgsConstructor
@EqualsAndHashCode(callSuper = true)
public class <%=entity.simpleName%> extends AbstractEntity {
    <% for(const field of entity.fields) {  %>
    private <%= field.type%> <%=field.name%> <% if(field.defaultValue){ %> = <%=field.typedValue()%> <% } %> ;
    <% } %>
}
`;
    let func = _.template(str);
    return func({entity});
}

function springBootController(entity) {
    let str = 
`
<%let varName = camelize(entity.simpleName);%>
<%let serviceName = varName + 'Service';%>

import lombok.extern.slf4j.Slf4j;
import org.springframework.web.bind.annotation.*;
import com.aincvy.stock.analysis.http.response.CommonResponse;

/**
 * Generated by sight(sight-entity-plus plugin) on <%=Date()%>
 */
@Slf4j
@RestController
@RequestMapping(IndexController.API_PREFIX )
public class <%=entity.simpleName%>Controller {

    private final <%=entity.simpleName%>Service <%=serviceName%>;

    public <%=entity.simpleName%>Controller(<%=entity.simpleName%>Service <%=serviceName%>){
        this.<%=serviceName%> = <%=serviceName%>;
    }

    // create 
    @PostMapping("/<%=varName%>")
    public CommonResponse create<%=entity.simpleName%>(@RequestBody <%=entity.simpleName%> entity){
        entity.setId(0);
        
        var id = <%=serviceName%>.create(entity);
        return CommonResponse.okWithLong(id);
    }

    // update
    @PatchMapping("/<%=varName%>/{id}")
    public CommonResponse update<%=entity.simpleName%>(@PathVariable Long id,@RequestBody <%=entity.simpleName%> entity){
        entity.setId(id);

        <%=serviceName%>.update(entity);
        return CommonResponse.OK;
    }

    // read(select)
    @GetMapping("/<%=varName%>/{id}")
    public Object select<%=entity.simpleName%>(@PathVariable Long id){
        var entity = <%=serviceName%>.select(id);
        if(entity == null){
            return CommonResponse.noData(id);
        }

        return entity;
    }

    // delete
    @DeleteMapping("/<%=varName%>/{id}")
    public CommonResponse delete<%=entity.simpleName%>(@PathVariable Long id){
        if (id == null) {
            return CommonResponse.ARGS_ERROR;
        }

        <%=serviceName%>.delete(id);
        return CommonResponse.OK;
    }
}


// ListController..

@GetMapping("/<%=varName%>/{pageSize}/{pageIndex}")
public Object query<%=entity.simpleName%>s(@PathVariable Integer pageSize, @PathVariable Integer pageIndex){
    return getList(pageSize, pageIndex, <%=varName%>Service::get<%=entity.simpleName%>List);
}

@GetMapping("/all/<%=varName%>")
public Object queryAll<%=entity.simpleName%>() {
    return <%=varName%>Service.getAll<%=entity.simpleName%>();
}

`;

    let func = _.template(str);
    return func({entity});
}

function springBootService(entity) {
    let interfaceStr = 
`

import java.util.List;

/**
 * Generated by sight(sight-entity-plus plugin) on <%=Date()%>
 */
public interface <%=entity.simpleName%>Service {

    long create(<%=entity.simpleName%> entity);

    long save(<%=entity.simpleName%> entity);

    void update(<%=entity.simpleName%> entity);

    <%=entity.simpleName%> select(long id);

    void delete(long id);

    void delete(<%=entity.simpleName%> entity);

    List\<<%=entity.simpleName%>\> get<%=entity.simpleName%>List(int startPos, int pageSize);

    List\<<%=entity.simpleName%>\> getAll<%=entity.simpleName%>();

}
`;

    let classStr = 
`
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.util.List;

import com.aincvy.stock.analysis.entity.EntityKeys;

/**
 * Generated by sight(sight-entity-plus plugin) on <%=Date()%>
 */
@Slf4j
@Service
public class <%=entity.simpleName%>ServiceImpl implements <%=entity.simpleName%>Service{

    @Override
    public long create(<%=entity.simpleName%> entity){
        if (entity.getId() <= 0) {
            entity.setId(EntityKeys.<%=entity.simpleName%>.nextId());
            entity.setCreateTime(System.currentTimeMillis());
            entity.setLastUpdateTime(entity.getCreateTime());
        }

        EntityKeys.<%=entity.simpleName%>.save(entity);
        return entity.getId();
    }

    @Override
    public long save(<%=entity.simpleName%> entity){
        return create(entity);
    }

    @Override
    public void update(<%=entity.simpleName%> entity){
        var old = select(entity.getId());
        if(old == null) {
            return ;
        }

        entity.setCreateTime(old.getCreateTime());
        entity.setLastUpdateTime(System.currentTimeMillis());
        create(entity);
    }

    @Override
    public <%=entity.simpleName%> select(long id){
        return EntityKeys.<%=entity.simpleName%>.load(id);
    }

    @Override
    public void delete(long id){
        EntityKeys.<%=entity.simpleName%>.delete(id);
    }

    @Override
    public void delete(<%=entity.simpleName%> entity){
        if(entity != null){
            delete(entity.getId());
        }
    }

    @Override
    public List\<<%=entity.simpleName%>\> get<%=entity.simpleName%>List(int startPos, int pageSize) {
        return EntityKeys.<%=entity.simpleName%>.loadPage(startPos,pageSize);
    }

    @Override
    public List\<<%=entity.simpleName%>\> getAll<%=entity.simpleName%>(){
        return EntityKeys.<%=entity.simpleName%>.loadAllDataReversed();
    }

}
`;

    let func = _.template(interfaceStr);
    let result = func({entity});
    result += "\n// Impl class. \n\n";
    func = _.template(classStr);
    result += func({entity});

    return result;
}

function entityKeysCode(entity){
    let str = 
`
    public static EntityKeyBuilder\<<%=entity.simpleName%>\> <%=entity.simpleName%> = null;

    // put this into function init's body.
    <%let varName = camelize(entity.simpleName);%>
    <%=entity.simpleName%> = new  EntityKeyBuilder\<\>("<%=varName%>.", redisTemplate);
    <%=entity.simpleName%>.idGenerator = new IdGenerator("<%=varName%>", redisTemplate);

    // 

`;

    let func = _.template(str);
    return func({ entity });
}



function springAllWithLombok(entity){
    let str = "// Java Entity With Lombok \n\n";
    str += javaEntityLombok(entity);
    str += "\n// Controller layer \n\n";
    str += springBootController(entity);
    str += "\n// Service layer  \n\n";
    str += springBootService(entity);
    str += "\n// EntityKeys layer  \n\n";
    str += entityKeysCode(entity);
    return str;
}


sight.entity.addOperation('java-entity-lombok', 'use lombok annotation', javaEntityLombok);
sight.entity.addOperation('java-spring-controller', 'spring-boot controller layer code', springBootController);
sight.entity.addOperation('java-spring-service', 'spring-boot service layer code', springBootService);
sight.entity.addOperation('java-entityKeys', 'entityKeys code(custom content)', entityKeysCode);
sight.entity.addOperation('java-spring-all', 'entity-with-lombok, spring boot controller,service, entityKeys', springAllWithLombok);

