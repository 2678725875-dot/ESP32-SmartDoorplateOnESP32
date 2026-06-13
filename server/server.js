const express = require('express');
const cors = require('cors');
const axios = require('axios');
const db = require('./db');

const app = express();
app.use(cors());
app.use(express.json());

//天气服务
// const WEATHER_API_KEY = '8dff6d16dd2f4bfe9c31cfbb4a85812c';
// const CITY = '101270101'; //成都

let cachedWeather = { temp: 20, text: '晴' };

// async function updateWeather() {
//   try {
//     const resp = await axios.get('https://devapi.qweather.com/v7/weather/now', {
//       params: {
//         location: CITY,
//         key: WEATHER_API_KEY
//       }
//     });
//     const now = resp.data.now;
//     cachedWeather = {
//       temp: now.temp,
//       text: now.text,
//       humidity: now.humidity
//     };
//     console.log('天气更新成功:', cachedWeather);
//   } catch (err) {
//     console.error('天气获取失败:', err.message);
//   }
// }
async function updateWeather() {
  try {
    const resp = await axios.get('https://api.openweathermap.org/data/2.5/weather', {
      params: {
        q: 'Chengdu',
        appid: 'YOUR_OPENWEATHER_API_KEY',  // 请替换为你自己的 OpenWeatherMap API Key
        units: 'metric'
      }
    });
    const data = resp.data;
    cachedWeather = {
      temp: Math.round(data.main.temp),
      text: data.weather[0].description,
      humidity: data.main.humidity
    };
    console.log('天气更新成功:', cachedWeather);
  } catch (err) {
    console.error('天气获取失败:', err.message);
  }
}

//首次启动时获取一次天气
updateWeather();
//每30分钟更新一次
setInterval(updateWeather, 30 * 60 * 1000);


// 获取天气
app.get('/api/weather', (req, res) => {
  res.json(cachedWeather);
});

//获取所有留言
app.get('/api/messages', (req, res) => {
  const rows = db.prepare('SELECT * FROM messages ORDER BY id DESC').all();
  res.json(rows);
});

//添加留言
app.post('/api/messages', (req, res) => {
  const { content } = req.body;
  if (!content || !content.trim()) {
    return res.status(400).json({ error: '留言内容不能为空' });
  }
  const stmt = db.prepare('INSERT INTO messages (content) VALUES (?)');
  const info = stmt.run(content.trim());
  res.json({ id: info.lastInsertRowid, content: content.trim() });
});

//删除留言
app.delete('/api/messages/:id', (req, res) => {
  const { id } = req.params;
  const stmt = db.prepare('DELETE FROM messages WHERE id = ?');
  const info = stmt.run(id);
  if (info.changes === 0) {
    return res.status(404).json({ error: '留言不存在' });
  }
  res.json({ success: true });
});

//获取访客计数
app.get('/api/visitors', (req, res) => {
  const row = db.prepare('SELECT count FROM visitor_count WHERE id = 1').get();
  res.json({ count: row ? row.count : 0 });
});

//记录一次访客
app.post('/api/visitors', (req, res) => {
  db.prepare('UPDATE visitor_count SET count = count + 1 WHERE id = 1').run();
  const row = db.prepare('SELECT count FROM visitor_count WHERE id = 1').get();
  console.log('访客+1，当前累计:', row.count);
  res.json({ count: row.count });
});


const PORT = 3000;
app.listen(PORT, () => {
  console.log(`后端服务已启动，端口: ${PORT}`);
});
