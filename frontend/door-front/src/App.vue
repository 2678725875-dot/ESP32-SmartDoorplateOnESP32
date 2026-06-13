<template>
  <div class="app-wrapper">
    <!-- 顶部渐变装饰 -->
    <div class="bg-glow"></div>

    <div class="app-container">
      <header class="header">
        <div class="logo-row">
          <span class="logo-icon">🚪</span>
          <h1 class="title">智能门牌</h1>
          <span class="badge">LIVE</span>
        </div>
        <p class="subtitle">留言板 · 实时数据看板</p>
      </header>

      <!-- ===== 数据卡片行 ===== -->
      <section class="stats-row">
        <!-- 天气卡片 -->
        <div class="stat-card weather-card" v-if="weather">
          <div class="stat-icon">🌤️</div>
          <div class="stat-body">
            <span class="stat-value">{{ weather.temp }}°</span>
            <span class="stat-label">{{ weather.text }}</span>
          </div>
          <div class="stat-extra">
            湿度 <strong>{{ weather.humidity }}%</strong>
          </div>
        </div>
        <div class="stat-card weather-card skeleton" v-else>
          <div class="stat-icon">🌤️</div>
          <div class="stat-body">
            <span class="stat-value">--°</span>
            <span class="stat-label">加载中...</span>
          </div>
        </div>

        <!-- 访客卡片 -->
        <div class="stat-card visitor-card">
          <div class="stat-icon">🚶</div>
          <div class="stat-body">
            <span class="stat-value">{{ visitorCount }}</span>
            <span class="stat-label">人经过门口</span>
          </div>
        </div>
      </section>

      <!--留言区-->
      <section class="messages-section">
        <div class="section-header">
          <h2>💬 留言板</h2>
          <span class="count-tag">{{ messages.length }} 条</span>
        </div>

        <!-- 留言列表-->
        <div class="message-cards" v-if="messages.length > 0">
          <TransitionGroup name="msg">
            <div class="msg-card" v-for="msg in messages" :key="msg.id">
              <div class="msg-header">
                <span class="msg-time">🕒 {{ msg.created_at }}</span>
                <button class="btn-delete" @click="deleteMessage(msg.id)">
                  ✕
                </button>
              </div>
              <p class="msg-content">{{ msg.content }}</p>
            </div>
          </TransitionGroup>
        </div>

        <!-- 空状态 -->
        <div class="empty-state" v-else>
          <span class="empty-icon">📭</span>
          <p>还没有留言，快来添加第一条吧</p>
        </div>
      </section>

      <!--添加留言 -->
      <section class="add-section">
        <div class="section-header">
          <h2>✏️ 添加留言</h2>
        </div>
        <div class="input-row">
          <input
            v-model="newMessage"
            type="text"
            placeholder="写下你想说的话..."
            maxlength="200"
            @keyup.enter="addMessage"
            class="msg-input"
          />
          <button
            class="btn-submit"
            :disabled="!newMessage.trim()"
            @click="addMessage"
          >
            发送 →
          </button>
        </div>
        <span class="char-count">{{ newMessage.length }}/200</span>
      </section>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import axios from 'axios'

const API_BASE = 'http://localhost:3000'

const weather = ref(null)
const messages = ref([])
const newMessage = ref('')
const visitorCount = ref(0)

const fetchWeather = async () => {
  try {
    const res = await axios.get(`${API_BASE}/api/weather`)
    weather.value = res.data
  } catch (err) {
    console.error('获取天气失败', err)
  }
}

const fetchMessages = async () => {
  try {
    const res = await axios.get(`${API_BASE}/api/messages`)
    messages.value = res.data
  } catch (err) {
    console.error('获取留言失败', err)
  }
}

const fetchVisitors = async () => {
  try {
    const res = await axios.get(`${API_BASE}/api/visitors`)
    visitorCount.value = res.data.count
  } catch (err) {
    console.error('获取访客计数失败', err)
  }
}

const addMessage = async () => {
  if (!newMessage.value.trim()) return
  try {
    await axios.post(`${API_BASE}/api/messages`, { content: newMessage.value.trim() })
    newMessage.value = ''
    await fetchMessages()
  } catch (err) {
    console.error('添加留言失败', err)
  }
}

const deleteMessage = async (id) => {
  if (!confirm('确定删除这条留言吗？')) return
  try {
    await axios.delete(`${API_BASE}/api/messages/${id}`)
    await fetchMessages()
  } catch (err) {
    console.error('删除留言失败', err)
  }
}

onMounted(() => {
  fetchWeather()
  fetchMessages()
  fetchVisitors()
  setInterval(fetchVisitors, 10000)
})
</script>

<style scoped>
/* ===== 主容器 ===== */
.app-container {
  position: relative;
  z-index: 1;
  max-width: 680px;
  margin: 0 auto;
  padding: 32px 20px 60px;
}

.header {
  text-align: center;
  margin-bottom: 28px;
}
.logo-row {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
}
.logo-icon { font-size: 32px; }
.title {
  font-size: 28px;
  font-weight: 700;
  color: #1e293b;
  letter-spacing: -0.5px;
}
.badge {
  font-size: 11px;
  font-weight: 700;
  background: linear-gradient(135deg, #6366f1, #8b5cf6);
  color: #fff;
  padding: 3px 9px;
  border-radius: 20px;
  letter-spacing: 1px;
}
.subtitle {
  margin-top: 6px;
  color: #94a3b8;
  font-size: 14px;
}

.stats-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 14px;
  margin-bottom: 24px;
}
@media (max-width: 500px) {
  .stats-row { grid-template-columns: 1fr; }
}

.stat-card {
  background: rgba(255,255,255,0.75);
  backdrop-filter: blur(14px);
  -webkit-backdrop-filter: blur(14px);
  border: 1px solid rgba(255,255,255,0.6);
  border-radius: 18px;
  padding: 18px 20px;
  display: flex;
  flex-direction: column;
  gap: 8px;
  box-shadow: 0 2px 12px rgba(0,0,0,0.04), 0 0 0 1px rgba(0,0,0,0.02);
  transition: transform 0.2s, box-shadow 0.2s;
}
.stat-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 8px 24px rgba(0,0,0,0.06);
}
.stat-card.skeleton { opacity: 0.6; }

.stat-icon { font-size: 28px; }
.stat-body {
  display: flex;
  align-items: baseline;
  gap: 10px;
}
.stat-value {
  font-size: 36px;
  font-weight: 700;
  color: #1e293b;
  line-height: 1;
}
.stat-label {
  font-size: 14px;
  color: #64748b;
  text-transform: capitalize;
}
.stat-extra {
  font-size: 13px;
  color: #94a3b8;
}
.stat-extra strong { color: #6366f1; }

.messages-section {
  background: rgba(255,255,255,0.7);
  backdrop-filter: blur(14px);
  -webkit-backdrop-filter: blur(14px);
  border: 1px solid rgba(255,255,255,0.6);
  border-radius: 20px;
  padding: 24px;
  margin-bottom: 20px;
  box-shadow: 0 2px 12px rgba(0,0,0,0.04);
}
.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16px;
}
.section-header h2 {
  font-size: 18px;
  font-weight: 600;
  color: #1e293b;
}
.count-tag {
  font-size: 13px;
  font-weight: 600;
  background: #eef2ff;
  color: #6366f1;
  padding: 4px 12px;
  border-radius: 20px;
}

.message-cards {
  display: flex;
  flex-direction: column;
  gap: 10px;
}
.msg-card {
  background: #fff;
  border: 1px solid #f1f5f9;
  border-radius: 14px;
  padding: 14px 18px;
  transition: all 0.2s;
}
.msg-card:hover {
  border-color: #e2e8f0;
  box-shadow: 0 4px 12px rgba(0,0,0,0.04);
}
.msg-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 6px;
}
.msg-time {
  font-size: 12px;
  color: #94a3b8;
}
.btn-delete {
  background: none;
  border: none;
  color: #cbd5e1;
  cursor: pointer;
  font-size: 16px;
  padding: 2px 6px;
  border-radius: 6px;
  transition: all 0.15s;
}
.btn-delete:hover {
  color: #ef4444;
  background: #fef2f2;
}
.msg-content {
  font-size: 15px;
  color: #334155;
  line-height: 1.6;
  word-break: break-word;
}

.empty-state {
  text-align: center;
  padding: 32px 0;
  color: #94a3b8;
}
.empty-icon { font-size: 40px; display: block; margin-bottom: 10px; }

/*消息动画*/
.msg-enter-active { animation: fadeSlide 0.35s ease; }
@keyframes fadeSlide {
  from { opacity: 0; transform: translateY(12px); }
  to   { opacity: 1; transform: translateY(0); }
}

.add-section {
  background: rgba(255,255,255,0.7);
  backdrop-filter: blur(14px);
  -webkit-backdrop-filter: blur(14px);
  border: 1px solid rgba(255,255,255,0.6);
  border-radius: 20px;
  padding: 24px;
  box-shadow: 0 2px 12px rgba(0,0,0,0.04);
}
.input-row {
  display: flex;
  gap: 10px;
}
.msg-input {
  flex: 1;
  padding: 14px 18px;
  font-size: 15px;
  border: 1.5px solid #e2e8f0;
  border-radius: 14px;
  background: #fff;
  color: #1e293b;
  outline: none;
  transition: border 0.2s, box-shadow 0.2s;
  font-family: inherit;
}
.msg-input:focus {
  border-color: #6366f1;
  box-shadow: 0 0 0 3px rgba(99,102,241,0.12);
}
.btn-submit {
  padding: 14px 22px;
  background: linear-gradient(135deg, #6366f1, #8b5cf6);
  color: #fff;
  font-size: 15px;
  font-weight: 600;
  border: none;
  border-radius: 14px;
  cursor: pointer;
  white-space: nowrap;
  transition: opacity 0.2s, transform 0.15s;
}
.btn-submit:hover { opacity: 0.9; transform: scale(1.03); }
.btn-submit:disabled { opacity: 0.4; cursor: not-allowed; transform: none; }

.char-count {
  display: block;
  text-align: right;
  font-size: 12px;
  color: #94a3b8;
  margin-top: 8px;
}
</style>
