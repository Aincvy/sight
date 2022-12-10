
function dataService(entity) {
    let varName = camelize(entity.simpleName);
    let constantName = upperConstantName(entity.simpleName);
    let hyphenName = lowerHyphenName(entity.simpleName);
    let str  = 
`
// put in api.service.js 
export const <%=entity.simpleName%>Service = {

    query<%=entity.simpleName%>(id, callback) {
        ApiService.query('<%=varName%>/' + id, callback);
    },

    create<%=entity.simpleName%>(params, callback) {
        ApiService.create('<%=varName%>', params, callback);
    },

    update<%=entity.simpleName%>(id, params, callback) {
        ApiService.update('<%=varName%>/' + id, params, callback);
    },

    delete<%=entity.simpleName%>(id, callback) {
        ApiService.delete('<%=varName%>/' + id, callback);
    },  

    list<%=entity.simpleName%> (pageIndex, callback, pageSize=50){
        ApiService.query('/list/' + \`<%=varName%>/\${pageSize}/\${pageIndex}\` , callback);
    },

    fetch<%=entity.simpleName%>(callback) {
        ApiService.query(\`/list/all/<%=varName%>\`, callback);
    },

};

// put in store.types.js
export const FETCH_<%=constantName%> = "fetch<%=entity.simpleName%>";
export const ADD_<%=constantName%> = "add<%=entity.simpleName%>";
export const DEL_<%=constantName%> = "del<%=entity.simpleName%>";
export const SET_<%=constantName%> = "set<%=entity.simpleName%>";
export const QUERY_<%=constantName%> = "query<%=entity.simpleName%>";
export const SET_<%=constantName%>_LIST = "set<%=entity.simpleName%>List";
export const PAGE_<%=constantName%> = "page<%=entity.simpleName%>";

// put in <%=varName%>.module.js

import { 
    FETCH_<%=constantName%>, 
    ADD_<%=constantName%>, 
    DEL_<%=constantName%>,
    SET_<%=constantName%>,
    SET_<%=constantName%>_LIST,
    QUERY_<%=constantName%>,
    PAGE_<%=constantName%>,
} from "./store.types";
import { <%=entity.simpleName%>Service } from "../common/api.service";
import { methods } from "../common/constants";
import ErrorCode from "../common/errorcode";
import Vue from "vue";

const state = {
    <%=varName%>List: [],
    <%=varName%>DataLoad: false,
    <%=varName%>Page: [],
}

const getters = {
    <%=varName%>List: state => state.<%=varName%>List,

    find<%=entity.simpleName%>: state => id => {
        id = parseInt(id);
        return state.<%=varName%>List.find( i => i.id === id);
    },

    <%=varName%>Page: state => state.<%=varName%>Page,
}

const actions = {
    [FETCH_<%=constantName%>]({commit, state}, pageIndex) {
        if(state.<%=varName%>DataLoad){
            return 
        }
        <%=entity.simpleName%>Service.fetch<%=entity.simpleName%>(function (data) {
            commit(SET_<%=constantName%>_LIST, data);
        });
    },
    [PAGE_<%=constantName%>]({commit, state}, pageIndex){
        <%=entity.simpleName%>Service.list<%=entity.simpleName%> (pageIndex, function (data) {
            commit(PAGE_<%=constantName%>, data);
        });
    },

    [DEL_<%=constantName%>]({commit}, {item, callback}) {
        <%=entity.simpleName%>Service.delete<%=entity.simpleName%>(item.id, function(data) {
            if(data.code == ErrorCode.OK) {
                commit(DEL_<%=constantName%>, item.id);
            }

            methods.callFunction(callback, {data, item});
        })
    },
    [ADD_<%=constantName%>]({commit}, {item, callback}) {
        <%=entity.simpleName%>Service.create<%=entity.simpleName%>(item, function(data){
            if(data.code === ErrorCode.OK) {
                item.id = data.paramLong;
                commit(ADD_<%=constantName%>, item);
            }

            methods.callFunction(callback, data);
        });
    },

    [SET_<%=constantName%>]({commit}, {item, callback}) {
        <%=entity.simpleName%>Service.update<%=entity.simpleName%>(item.id, item, function(data){
            if(data.code === ErrorCode.OK) {
                commit(SET_<%=constantName%>, item);
            }

            methods.callFunction(callback, data);
        });
    },
    [QUERY_<%=constantName%>]({commit}, id){
        <%=entity.simpleName%>Service.query<%=entity.simpleName%>(id, function(data){
            if (typeof data.code !== 'undefined') {
                console.log(JSON.stringify(data));
            } else {
                commit(SET_<%=constantName%>, data);
            }
        });
    },
}

const mutations = {
    [SET_<%=constantName%>_LIST](state, data) {
        state.<%=varName%>List = data;
        state.<%=varName%>DataLoad = true;
    },

    [DEL_<%=constantName%>](state, id){
        let index = state.<%=varName%>List.findIndex(i => i.id === id);
        if(index >= 0) {
            state.<%=varName%>List.splice(index, 1);
        }

        index = state.<%=varName%>Page.findIndex(i => i.id === id);
        if (index >= 0) {
            state.<%=varName%>Page.splice(index, 1);
        }
    },

    [ADD_<%=constantName%>](state, item) {
        state.<%=varName%>List.push(item);
        state.<%=varName%>Page.push(item);
    },

    [SET_<%=constantName%>](state, item) {
        let index = state.<%=varName%>List.findIndex(i => i.id === item.id);
        if(index >= 0) {
            // state.<%=varName%>List[index] = item;
            Vue.set(state.<%=varName%>List, index, item);
        } else  {
            state.<%=varName%>List.push(item);
        }
    },

    [PAGE_<%=constantName%>](state, list) {
        state.<%=varName%>Page = list;
    }

}

export default {
    state,
    getters,
    actions,
    mutations,
};


// put in router/index.js

{
    path: "<%=hyphenName%>-list",
    name: "view-<%=hyphenName%>-list",
    component: () => import("@/components/<%=varName%>/List<%=entity.simpleName%>"),
    meta: {
        requiresAuth: true
    }
}, 
{
    path: "<%=hyphenName%>-edit/:editId",
    name: "view-<%=hyphenName%>-edit",
    component: () => import("@/components/<%=varName%>/Edit<%=entity.simpleName%>"),
    props: true,
    meta: {
        requiresAuth: true
    }
}, 

// put in store/index.js
import <%=varName%> from './<%=varName%>.module'
`;

    let func = _.template(str);
    return func({entity, varName, constantName, hyphenName});

}


function listVueComponents(entity) {
    let hyphenName = lowerHyphenName(entity.simpleName);
    let varName = camelize(entity.simpleName);
    let constantName = upperConstantName(entity.simpleName);
    let str = 
`
<template>
    <div>
        <router-link :to="{ name: 'view-<%=hyphenName%>-edit' }">
            <button class="ui primary button">新建</button>
        </router-link>

        <h1>列表</h1>
        <table class="ui celled table">
            <thead>
                <tr>
                    <th>Id</th>
                    <% for(const field of entity.fields) {  %>
                        <th><%=field.name%></th>
                    <% } %>
                    <th>操作</th>
                </tr>
            </thead>
            <tbody>
                <tr v-for="item in <%=varName%>Page" :key="item.id">
                    <td>{{ item.id }}</td>
                    <% for(const field of entity.fields) {  %>
                        <td>{{ item.<%=field.name%> }}</td>
                    <% } %>
                    <td>
                        <div class="ui tiny icon buttons">
                            <button class="ui button" @click="askDelete<%=entity.simpleName%>(item)" title="delete">
                                <i class="trash icon"></i>
                            </button>

                            <button class="ui button" @click="routeToEdit(item)" title="edit">
                                <i class="edit icon"></i>
                            </button>
                            <button class="ui button" @click="duplicate(item)" title="duplicate">
                                <i class="copy icon"></i>
                            </button>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
        
        <div>
            <div class="ui icon buttons" style="margin-right: 10px;">
                <button class="ui button" title="prePage" :disabled="(pageIndex == 0)" @click="prePage">
                    <i class="angle left icon"></i>
                </button>
                <button class="ui button">
                    {{ pageIndex }}
                </button>
                <button class="ui button" title="nextPage" @click="nextPage">
                    <i class="angle right icon"></i>
                </button>
            </div>

            <div class="ui action input">
                <input type="number" step="1" v-model="toPageIndex" placeholder="Input Page...">
                <button class="ui button" @click="toPage">
                    <i class="angle double right icon"></i>
                </button>
            </div>
        </div>

        <CommonDialog :dialog-holder="initDialog" dialog-type="Remove">
        </CommonDialog>
    </div>
</template>

<script>

import {
    FETCH_<%=constantName%>,
    DEL_<%=constantName%>,
    ADD_<%=constantName%>, 
    PAGE_<%=constantName%>,

} from "@/store/store.types";

import CommonDialog from "@/components/modal/CommonDialog";
import { mapGetters } from 'vuex';
import ErrorCode from "../../common/errorcode";

export default {
    name: 'List<%=entity.simpleName%>',
    components: {
        CommonDialog
    },
    data() {
        return {
            dialogHolder: null,
            operatingItem: null,
            pageIndex: 0,
            toPageIndex: 0,
        }
    },

    mounted() {
        // this.$store.dispatch(FETCH_<%=constantName%>, 0);
        this.fetchPageData();
    },

    computed: {
        ...mapGetters(['<%=varName%>Page']),

    },

    methods: {
        fetchPageData() {
            this.$store.dispatch(PAGE_<%=constantName%>, this.pageIndex);
        },

        initDialog(dialog) {
            this.dialogHolder = dialog;
        },

        askDelete<%=entity.simpleName%>(item) {
            this.operatingItem = item;
            let updateTime = this.unixTimestampString(item.lastUpdateTime);
            let content = \`id: \${item.id}, 最后更新时间: \${updateTime}\`;
            this.dialogHolder.setContent("是否删除？", content);
            this.dialogHolder.showDialog(this.delete<%=entity.simpleName%>);
        },

        delete<%=entity.simpleName%>(drop) {
            if (this.operatingItem == null) {
                this.dialogHolder.toastWarning('need select one item first!');
                return;
            }

            let item = this.operatingItem;

            console.log("will delete <%=entity.simpleName%>: " + item.id)
            this.$store.dispatch(DEL_<%=constantName%>, { item, callback: this.delete<%=entity.simpleName%>Callback });
        },

        delete<%=entity.simpleName%>Callback({ data, item }) {
            if (data.code === ErrorCode.OK) {
                this.toastSuccess(\`删除\${item.id}成功\`);
            } else {
                this.toastError(\`删除\${item.id}失败， 原因是: \${data.msg}\`);
            }
        },

        routeToEdit(item)  {
            this.$router.push({
                name: 'view-<%=hyphenName%>-edit',
                params: {
                    editId: item.id
                }
            });
        },

        duplicate(item) {
            let copy = JSON.parse(JSON.stringify(item));
            copy.id = 0;

            this.$store.dispatch(ADD_<%=constantName%>, {
                item: copy,
                callback: undefined
            });
        },

        nextPage() {
            this.pageIndex++;
            this.fetchPageData();
        },

        prePage() {
            if (this.pageIndex > 0) {
                this.pageIndex--;
                this.fetchPageData();
            }
        },
        toPage(){
            if (this.toPageIndex < 0) {
                this.toastError(this.toPageIndex + ' 不是一个有效的页码');
                return;
            }

            this.pageIndex = this.toPageIndex;
            this.fetchPageData();
        },
    },

};

</script>

<style scoped>

</style>

`;

    let func = _.template(str);
    return func({entity, hyphenName, constantName, varName});
}

function editVueComponents(entity){
    let varName = camelize(entity.simpleName);
    let constantName = upperConstantName(entity.simpleName);
    let hyphenName = lowerHyphenName(entity.simpleName);
    let str = 
`
<template>
    <div class="ui form">
        <h1>{{ titleText }}</h1>
        <div class="ui field">
            <label>ID：</label>
            <input type="text" readonly="readonly" v-model="<%=varName%>.id" placeholder="id">
        </div>

        <% for(const field of entity.fields) {  %>
            <div class="ui field" :class="{ error: status.<%=field.name%> }">
                <label><%=field.name%>: </label>
                <input type="text" v-model="<%=varName%>.<%=field.name%>" placeholder="<%=field.name%>">
            </div>
        <% } %>

        <div>
            <button class="ui primary button" @click="createItem">{{ createButtonText }}</button>
            <button class="ui primary button" @click="resetForm">Reset</button>
            <button class="ui primary button" @click="resetStatus">Reset Status</button>

            <router-link :to="{ name: 'view-<%=hyphenName%>-list' }">
                <button class="ui button">列表</button>
            </router-link>
        </div>

        <div>
            <!-- 状态栏 -->
            <p v-if="status.showSuccess"><span class="ui success text">成功创建/更新 <em
                        data-emoji=":heavy_check_mark:"></em></span></p>
            <p v-if="status.errorText"><span class="ui error text">{{ status.errorText }} </span></p>
        </div>

    </div>
</template>

<script>

import ErrorCode from '../../common/errorcode';

import {
    ADD_<%=constantName%>,
    SET_<%=constantName%>,
} from '../../store/store.types'

export default {
    name: "Edit<%=entity.simpleName%>",
    data() {
        return {
            <%=varName%>: {
                id: 0,
                <% for(const field of entity.fields) {  %>
                    <%=field.name%>: null, 
                <% } %>
            },
            status: {
                showSuccess: false,
                errorText: '',
                <% for(const field of entity.fields) {  %>
                    <%=field.name%>: false, 
                <% } %>
            },
        };
    },

    props: {
        editId: {
            required: false,
            type: [Number, String],
            default: 0,
        }
    },

    mounted() {
        console.log("edit<%=entity.simpleName%> mount: " + this.editId);
        if (this.editId && this.editId > 0) {
            // in edit mode
            let find<%=entity.simpleName%> = this.$store.getters.find<%=entity.simpleName%>;
            var item = find<%=entity.simpleName%>(this.editId);
            if (!item) {
                this.status.errorText = \`无法找到id为\${this.editId}的数据， 也许你应该先返回列表\`;
            } else {
                this.<%=varName%> = JSON.parse(JSON.stringify(item))
            }
        }
    },

    methods: {
        createItem() {
            let <%=varName%> = this.<%=varName%>;
            this.resetStatus();

            let status = this.status;

            let isEditItem = this.isEditItem;

            <% for(const field of entity.fields) {  %>
                if(!<%=varName%>.<%=field.name%>) {
                    status.<%=field.name%> = true;
                    return ;
                }
            <% } %>


            function callback(data) {
                if (data.code === ErrorCode.OK) {
                    status.showSuccess = true;
                    if(!isEditItem) {
                        <%=varName%>.id = data.paramLong;
                    }
                } else {
                    status.errorText = JSON.stringify(data);
                }
            }

            if (isEditItem) {
                // edit 
                this.$store.dispatch(SET_<%=constantName%>, {
                    item: <%=varName%>,
                    callback: callback
                })
            } else {
                // create
                this.$store.dispatch(ADD_<%=constantName%>, {
                    item: <%=varName%>,
                    callback: callback
                });
            }
        },

        resetForm() {
            let <%=varName%> = this.<%=varName%>;
            <% for(const field of entity.fields) {  %>
                <%=varName%>.<%=field.name%> = null;
            <% } %>
        },

        resetStatus() {
            let status = this.status;
            status.showSuccess = false;
            status.errorText = '';

            <% for(const field of entity.fields) {  %>
                status.<%=field.name%> = false;
            <% } %>
        },
    },

    computed: {
        titleText() {
            if (this.editId && this.editId > 0) {
                return "编辑"
            } else {
                return "创建新的元素"
            }
        },

        createButtonText() {
            if (this.editId && this.editId > 0) {
                return "Update"
            } else {
                return "Create"
            }
        },

        isEditItem() {
            return this.editId && this.editId > 0;
        },

    }
};

</script>

<style scoped>

</style>
`;

    let func = _.template(str);
    return func({entity, hyphenName, constantName, varName});

}

sight.entity.addOperation('vue-api-func', 'api.service.js function', dataService);
sight.entity.addOperation('vue-list-template', 'ListXXX Vue Components function', listVueComponents);
sight.entity.addOperation('vue-edit-template', 'EditXXX Vue Components function', editVueComponents);
