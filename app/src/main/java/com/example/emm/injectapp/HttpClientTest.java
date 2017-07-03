package com.example.emm.injectapp;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Created by gpf on 17-7-3.
 */

public class HttpClientTest {

    public void  get(String strUrl) {
        URL url = null;
        HttpURLConnection conn = null;
        String requestHeader = null;//请求头
        byte[] requestBody = null;//请求体
        String responseHeader = null;//响应头
        byte[] responseBody = null;//响应体

        try {

            //发送GET请求
            url = new URL(strUrl);
            conn = (HttpURLConnection) url.openConnection();

            //HttpURLConnection默认就是用GET发送请求，所以下面的setRequestMethod可以省略
            conn.setRequestMethod("GET");

            //HttpURLConnection默认也支持从服务端读取结果流，所以下面的setDoInput也可以省略
            conn.setDoInput(true);

            //禁用网络缓存
            conn.setUseCaches(false);

            //获取请求头
            requestHeader = getReqeustHeader(conn);

            //在对各种参数配置完成后，通过调用connect方法建立TCP连接，但是并未真正获取数据
            //conn.connect()方法不必显式调用，当调用conn.getInputStream()方法时内部也会自动调用connect方法
            conn.connect();

            //获取响应头
            responseHeader = getResponseHeader(conn);

        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            //最后将conn断开连接
            if (conn != null) {
                conn.disconnect();
            }
        }

        System.out.println("请求头: ");
        System.out.println(requestHeader);

        System.out.println("响应头: ");
        System.out.println(responseHeader);
        System.out.println("");
    }

    //读取请求头
    private String getReqeustHeader(HttpURLConnection conn) {
        //https://github.com/square/okhttp/blob/master/okhttp-urlconnection/src/main/java/okhttp3/internal/huc/HttpURLConnectionImpl.java#L236
        Map<String, List<String>> requestHeaderMap = conn.getRequestProperties();
        Iterator<String> requestHeaderIterator = requestHeaderMap.keySet().iterator();
        StringBuilder sbRequestHeader = new StringBuilder();
        while (requestHeaderIterator.hasNext()) {
            String requestHeaderKey = requestHeaderIterator.next();
            String requestHeaderValue = conn.getRequestProperty(requestHeaderKey);
            sbRequestHeader.append(requestHeaderKey);
            sbRequestHeader.append(":");
            sbRequestHeader.append(requestHeaderValue);
            sbRequestHeader.append("\n");
        }
        return sbRequestHeader.toString();
    }

    //读取响应头
    private String getResponseHeader(HttpURLConnection conn) {
        Map<String, List<String>> responseHeaderMap = conn.getHeaderFields();
        int size = responseHeaderMap.size();
        StringBuilder sbResponseHeader = new StringBuilder();
        for(int i = 0; i < size; i++){
            String responseHeaderKey = conn.getHeaderFieldKey(i);
            String responseHeaderValue = conn.getHeaderField(i);
            sbResponseHeader.append(responseHeaderKey);
            sbResponseHeader.append(":");
            sbResponseHeader.append(responseHeaderValue);
            sbResponseHeader.append("\n");
        }
        return sbResponseHeader.toString();
    }
}
