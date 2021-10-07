// 
addTemplateNode({
    __meta_name: "RedisClient",
    __meta_address: "dao/redis",
    __meta_options: {
        enter: true,
    },
    __meta_func: {
        generateCodeWork($) {
            var redis = require('redis');
            var client = redis.createClient($.port(), $.host());
        }, 
        
        onReverseActive($, $$) {
            return client;
        },
    },
    // other ideas
    __meta_inputs: {
        host: {
            type: 'string',
            defaultValue: '127.0.0.1',
            showValue: true,
        },
        port: {
            type: 'int',
            defaultValue: 6379,
            showValue: true,
        }, 

        errorHandle: {
            type: 'function',
        }
    },
    __meta_outputs: {
        client: 'RedisClient',
    },

});

// 
addTemplateNode({
    __meta_name: "RedisKey",
    __meta_address: "dao/redis",
    __meta_options: {
        enter: true,
    },
    __meta_func: {
        generateCodeWork($) {
            // await $.client() .set($.key(), $.value());
        },

    },
    // other ideas
    __meta_inputs: {
        // client should be more intelligence
        client: 'RedisClient',
        key: {
            type: 'string',
            showValue: true,
        },
        value: {
            type: 'object',
            showValue: false,
        }
    },
    __meta_outputs: {
        
    },

    type: {
        type: 'string',   // this should be a combo box
    },

});